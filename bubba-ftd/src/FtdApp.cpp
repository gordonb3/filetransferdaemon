/*
    
    DownloadManager - http://www.excito.com/
    
    FtdApp.cpp - this file is part of DownloadManager.
    
    Copyright (C) 2007 Tor Krill <tor@excito.com>
    
    DownloadManager is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    DownloadManager is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with DownloadManager; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id: FtdApp.cpp 2026 2008-10-16 10:12:57Z tor $
*/

#include "FtdApp.h"

#include <utility>
#include <iostream>
#include <sstream>
#include <syslog.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sigc++/sigc++.h>
#include <time.h>

#include "FtdConfig.h"
#include <libeutils/FileUtils.h>
#include <libeutils/UserGroups.h>
#include <libeutils/DeferredWork.h>

#include "CurlDownloader.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


/*
 * 
 * FTD implementation
 * 
 */

bool FtdApp::FindByUUID(string& uuid, string& user){
	bool ret=false;
	transfermutex.Lock();
	list<Downloader*>::iterator lIt=this->transfers[user].begin();

	while (lIt!=this->transfers[user].end() && (*lIt)->GetUUID()!=uuid ) {
		++lIt;
	}

	if (lIt!=this->transfers[user].end()) {
		ret = true;
	}else{
		ret = false;
	}
	transfermutex.Unlock();
	return ret;
}

bool FtdApp::FindByURL(string& url){

	transfermutex.Lock();

	map<string,list<Downloader*> >::iterator uIt=this->transfers.begin();
	while(uIt!=this->transfers.end()){

		list<Downloader*>::iterator lIt=(*uIt).second.begin();

		while (lIt!=(*uIt).second.end()) {
			if( (*lIt)->GetUrl()==url){
				transfermutex.Unlock();
				return true;
			}
			++lIt;
		}
		++uIt;
	}
	transfermutex.Unlock();
	return false;
}

string FtdApp::GetDownloadDir(const string& user){
    string path="/home/"+user+"/";

    struct stat st;
    if (stat(path.c_str(),&st)<0) {
        syslog(LOG_ERR,"Unable to stat home directory (%s): %m",path.c_str());
		throw std::runtime_error("Unable to stat home directory");
    }

    struct passwd* pwd=getpwnam(user.c_str());
    if(pwd==NULL){
	    syslog(LOG_ERR,"User:%s not found: %m",user.c_str());
		throw std::runtime_error("User not found");
    }

    struct group* grp=getgrnam("users");
    if (grp==NULL) {
    	syslog(LOG_ERR,"users group not found");
		throw std::runtime_error("Group not found");
    }
	string dldir="downloads";
	try{
		FtdConfig& cfg=FtdConfig::Instance();
		dldir=cfg.GetString("general","downloaddir");
	}catch(std::runtime_error& err){
		syslog(LOG_NOTICE,"Unable to read download dir, using default");
	}
	
    path+=dldir;
    if (stat(path.c_str(),&st)<0) {

	   syslog(LOG_NOTICE,"No download directory present creating");
	   if (mkdir(path.c_str(),0770)) {
		   syslog(LOG_ERR,"Failed to create download directory: %m");
			throw std::runtime_error("Failed to create download directory");
	   }

	   if (chown(path.c_str(),pwd->pw_uid,grp->gr_gid)) {
		   syslog(LOG_NOTICE,"Failed to chown download directory");
	   }

    }
    return path;
}

void FtdApp::AddDownload(Json::Value& req, UnixClientSocket* sock){
    struct stat st;

	// Validate input
	
	if(!(req.isMember("user") && req["user"].isString())){
		syslog(LOG_ERR,"AddDownload: missing or invalid user");
	    this->SendFail(sock);
		return;
	}

	if(!(req.isMember("url") && req["url"].isString())){
		syslog(LOG_ERR,"AddDownload: missing or invalid url");
	    this->SendFail(sock);
		return;
	}

	if(!(req.isMember("uuid") && req["uuid"].isString())){
		syslog(LOG_ERR,"AddDownload: missing or invalid uuid");
	    this->SendFail(sock);
		return;
	}

	if(!(req.isMember("policy") && req["policy"].isIntegral())){
		syslog(LOG_ERR,"AddDownload: missing or invalid policy");
	    this->SendFail(sock);
		return;
	}

	string user=req["user"].asString();
	string uuid=req["uuid"].asString();
	string url=req["url"].asString();
	unsigned int policy= req["policy"].asUInt();


	// Check for duplicate downloads doubleclicks etc
	if (this->FindByUUID(uuid,user)){
		this->SendFail(sock);
		syslog(LOG_NOTICE,"Duplicate uuid");
		return;
	}

	// Avoid duplicate concurrent downloads
	if(this->FindByURL(url)){
	    this->SendFail(sock);
		syslog(LOG_NOTICE,"Duplicate URL");
		return;
	}

    string filename=url;
    filename=filename.substr(filename.rfind("/")+1);

	string path=this->GetDownloadDir(user);

    if (!stat( (path+filename).c_str(),&st)) {
        int i=0;
        stringstream oss;
        do{
            i++;
            oss.str("");
            oss<<filename<<"("<<i<<")";
        
        }while(!stat((path+oss.str()).c_str(),&st));
        filename=oss.str();
    }

    URL turl(url);
    Downloader* down=this->FindDownloader(turl);

    if(down!=NULL){
	    down->SetUrl(url);
	    down->SetDestinationPath(path);
	    down->SetDownloadName(filename);
	    down->SetUUID(uuid);
	    down->SetGroup(Group::GroupToGID("users"));
	    down->SetUser(User::UserToUID(user));
	    down->SetPolicy(policy);

	    if(policy && DLP_AUTOREMOVE){
	    	down->SignalDone.connect(sigc::bind(sigc::mem_fun(this,&FtdApp::dlDone),down));
	    	down->SignalFailed.connect(sigc::hide<0>(sigc::bind(sigc::mem_fun(this,&FtdApp::dlDone),down)));
	    }

		this->RegisterDownload(user,down);
	    this->SendOk(sock);

	    down->StartDownload();
	    syslog(LOG_DEBUG, "User %s added download",user.c_str());
    }else{
	    this->SendFail(sock);
	    syslog(LOG_ERR, "Failed to find suitable downloader"); 
    }

}

void FtdApp::RegisterDownload(const string& user,Downloader* down){
	transfermutex.Lock();
	this->transfers[user].push_back(down);
	transfermutex.Unlock();
}


DownloadManager* FtdApp::FindManagerByService(const string& service){
	DownloadManager* dm=NULL;
	for(list<DownloadManager*>::iterator iDM=this->managers.begin();
		iDM!=this->managers.end();++iDM){
			if( (*iDM)->ProvidesService(service)){
				dm=*iDM;
			}
	}
	return dm;
}

void FtdApp::GetDownloadThrottle(Json::Value& req, UnixClientSocket* sock){

	// Validate input
	
	if(!(req.isMember("name") && req["name"].isString())){
		syslog(LOG_ERR,"GetDownload throttle: missing or invalid service name");
	    this->SendFail(sock);
		return;
	}
	
	string name=req["name"].asString();
	
	DownloadManager* dm=this->FindManagerByService(name);
	if(dm){
		Json::Value rep(Json::objectValue);
		rep["cmd"]=GET_DOWNLOAD_THROTTLE;
		rep["name"]=name;
		rep["size"]=dm->GetDownloadThrottle();
		this->SendJsonValue(rep,sock);
	}else{
	    this->SendFail(sock);
	}
	
}

void FtdApp::SetDownloadThrottle(Json::Value& req, UnixClientSocket* sock){
	// Validate input
	if(!(req.isMember("name") && req["name"].isString())){
		syslog(LOG_ERR,"SetDownload throttle: missing or invalid service name");
	    this->SendFail(sock);
		return;
	}
	if(!(req.isMember("size") && req["size"].isIntegral())){
		syslog(LOG_ERR,"SetDownload throttle: missing or invalid throttle");
	    this->SendFail(sock);
		return;
	}
	string name=req["name"].asString();
	unsigned int size;
	
	try{
		size=req["size"].asInt();
	}catch(std::runtime_error& err){
		syslog(LOG_ERR,"SetDownload throttle: invalid argument");
	    this->SendFail(sock);
		return;
	}
	
	DownloadManager* dm=this->FindManagerByService(name);
	if(dm){
		dm->SetDownloadThrottle(size);
		this->SendOk(sock);
	}else{
		this->SendFail(sock);
	}
}

void FtdApp::GetUploadThrottle(Json::Value& req, UnixClientSocket* sock){
	// Validate input
	
	if(!(req.isMember("name") && req["name"].isString())){
		syslog(LOG_ERR,"GetUpload throttle: missing or invalid service name");
	    this->SendFail(sock);
		return;
	}
	
	string name=req["name"].asString();
	
	DownloadManager* dm=this->FindManagerByService(name);
	if(dm){
		Json::Value rep(Json::objectValue);
		rep["cmd"]=GET_UPLOAD_THROTTLE;
		rep["name"]=name;
		rep["size"]=dm->GetUploadThrottle();
		this->SendJsonValue(rep,sock);
	}else{
	    this->SendFail(sock);
	}
}

void FtdApp::SetUploadThrottle(Json::Value& req, UnixClientSocket* sock){
	// Validate input
	if(!(req.isMember("name") && req["name"].isString())){
		syslog(LOG_ERR,"SetUploadThrottle: missing or invalid service name");
	    this->SendFail(sock);
		return;
	}
	if(!(req.isMember("size") && req["size"].isIntegral())){
		syslog(LOG_ERR,"SetUploadThrottle: missing or invalid throttle");
	    this->SendFail(sock);
		return;
	}
	string name=req["name"].asString();
	unsigned int size;
	
	try{
		size=req["size"].asInt();
	}catch(std::runtime_error& err){
		syslog(LOG_ERR,"SetUploadThrottle: invalid argument");
	    this->SendFail(sock);
		return;
	}

	DownloadManager* dm=this->FindManagerByService(name);
	if(dm){
		dm->SetUploadThrottle(size);
		this->SendOk(sock);
	}else{
		this->SendFail(sock);
	}

}



void FtdApp::dlDone(void* dl){
	Downloader* downloader=static_cast<Downloader*>(dl);
	
	struct passwd* pwd=getpwuid(downloader->GetUser());
    if(pwd==NULL){
	    syslog(LOG_ERR,"User not found: %d",downloader->GetUser());
	    return;
    }	
    Json::Value rep(Json::objectValue);
    rep["user"]=pwd->pw_name;
    rep["url"]=downloader->GetUrl();
    rep["uuid"]=downloader->GetUUID();
	this->CancelDownload(rep);
	
}

void FtdApp::RemoveDownload(Downloader* dl){
	
	const struct timespec delay = {0,100000000L};

	syslog(LOG_DEBUG,"Remove download: %s",dl->GetDownloadName().c_str());
	
	string user=User::UIDToUser(dl->GetUser());
	transfermutex.Lock();
	list<Downloader*>::iterator lIt=this->transfers[user].begin();

    while (lIt!=this->transfers[user].end() && ((*lIt)->GetUrl()!=dl->GetUrl()) ) {
        ++lIt;
    }

    if (lIt!=this->transfers[user].end()) {
    	while(!dl->ReapOK()){
		nanosleep(&delay, NULL);
    	}
    	this->transfers[user].erase(lIt);
    	delete(dl);
    }else{
    	syslog(LOG_ERR,"FtdApp::RemoveDownload: Download not found");
    }
    transfermutex.Unlock();

}

void FtdApp::DeferredRemove(void *dl){
	if(!dl){
		syslog(LOG_ERR,"Deferred remove called with NULL parameter");
		return;
	}
	pair<FtdApp*,Downloader*> *pl=static_cast< pair<FtdApp*,Downloader*>* >(dl);
	pl->first->RemoveDownload(pl->second);
	
	delete pl;
}

void FtdApp::Remove(void* dl){
	// Cant delete download from this context. Add to deferred work queue.
	DeferredWork::Instance().AddWork(FtdApp::DeferredRemove,dl);
}

// We unfortunately cant send any reply here since this
// method is called async from download complete

void FtdApp::CancelDownload(Json::Value& req){
	
	// Validate input
	if(!(req.isMember("user") && req["user"].isString())){
		syslog(LOG_ERR,"CancelDownload: missing or invalid user");
		return;
	}

	if(!(req.isMember("url") && req["url"].isString())){
		syslog(LOG_ERR,"CancelDownload: missing or invalid url");
		return;
	}

	if(!(req.isMember("uuid") && req["uuid"].isString())){
		syslog(LOG_ERR,"CancelDownload: missing or invalid uuid");
		return;
	}

	string user=req["user"].asString();
	string uuid=req["uuid"].asString();
	string url=req["url"].asString();


	syslog(LOG_DEBUG,"Cancel download %s",url.c_str());
	
	transfermutex.Lock();
	list<Downloader*>::iterator lIt=this->transfers[user].begin();
	bool has_uuid=uuid.length()!=0;
	
	// First try to find it by uuid
	if(has_uuid){
		while (lIt!=this->transfers[user].end() && (*lIt)->GetUUID()!=uuid ) {
			++lIt;
		}
	}else{
		syslog(LOG_DEBUG,"No UUID provided");
	}

	// If that fail try by url
	if (!has_uuid || lIt==this->transfers[user].end()) {
		syslog(LOG_DEBUG,"Failed to locate download by uuid, trying url instead");
		lIt=this->transfers[user].begin();
		while (lIt!=this->transfers[user].end() && (*lIt)->GetUrl()!=url ) {
			++lIt;
		}
	}

	if (lIt!=this->transfers[user].end()) {
		syslog(LOG_DEBUG,"Found download to cancel: %s",(*lIt)->GetUrl().c_str());

    	// Wait for completion before remove
    	pair<FtdApp*,Downloader*> *pl=new pair<FtdApp*,Downloader*>(this,*lIt);
    	if(!(*lIt)->RegisterCompletionCallback(FtdApp::Remove,pl)){
    		// Download already completed, remove immediately
    		// Have to release lock since it is acquired in called function.
    		// TODO: perhaps use a recursive lock to not having to do this ugly
    		// construction.
    		transfermutex.Unlock();
    		FtdApp::DeferredRemove(pl);
    		transfermutex.Lock();
    	}else{
    		(*lIt)->CancelDownload();
    	}
        
	}else{
		syslog(LOG_ERR,"Could not find download to cancel");
	}
	transfermutex.Unlock();
	
	syslog(LOG_DEBUG, "User %s canceled download",user.c_str());
}

void FtdApp::SendDownload(const string& user, Downloader* dl,UnixClientSocket* sock){
	
	Json::Value rep(Json::objectValue);

	rep["cmd"]=LIST_DOWNLOADS;
	rep["url"]=dl->GetUrl();
	rep["user"]=user;
	rep["size"]=Json::Value::ULong(dl->Size());
	rep["downloaded"]=Json::Value::ULong(dl->Downloaded());
	rep["uuid"]=dl->GetUUID();
	rep["status"]=Json::Value::Int(dl->GetStatus());
	rep["name"]=dl->GetDownloadName();
	rep["info"]=dl->GetInfo();
	rep["policy"]=Json::Value::Int(dl->GetPolicy());

	this->SendJsonValue(rep,sock);
}

void FtdApp::ListUserDownloads(const string& user, UnixClientSocket* sock){

    if (transfers.find(user)!=transfers.end()) {
        list<Downloader*>::iterator lIt=transfers[user].begin();
        
        while(lIt!=transfers[user].end()){
			this->SendDownload(user,*lIt, sock);
			
            ++lIt;
        }
    }

}

void FtdApp::ListDownloads(Json::Value& req, UnixClientSocket* sock){

	// Validate input
	if(!(req.isMember("user") && req["user"].isString())){
		syslog(LOG_ERR,"ListDownloads: missing or invalid user");
	    this->SendFail(sock);
		return;
	}

	if(!(req.isMember("uuid") && req["uuid"].isString())){
		syslog(LOG_ERR,"ListDownloads: missing or invalid uuid");
	    this->SendFail(sock);
		return;
	}

	string user=req["user"].asString();
	string uuid=req["uuid"].asString();

	if(uuid!=""){
		// List one UUID
		this->GetDownloadByUUID(user,uuid,sock);
		return; // Dont send more ok/fail
	}else if (user=="*") {
        // List all

		transfermutex.Lock();

        map<string,list<Downloader*> >::iterator tIt=this->transfers.begin();
        while(tIt!=this->transfers.end()){
            cout << "List :"<<(*tIt).first<<endl;
            this->ListUserDownloads((*tIt).first,sock);
            ++tIt;
        }
    	
        transfermutex.Unlock();

    }else{
        // List specific user
    	transfermutex.Lock();

        this->ListUserDownloads(user,sock);

        transfermutex.Unlock();
    }


    this->SendOk(sock);
}

void FtdApp::GetDownloadByUUID(string& user, string& uuid, UnixClientSocket* sock){

	transfermutex.Lock();

	list<Downloader*>::iterator lIt=this->transfers[user].begin();

	while (lIt!=this->transfers[user].end() && (*lIt)->GetUUID()!=uuid ) {
		++lIt;
	}

	if (lIt!=this->transfers[user].end()) {
		this->SendDownload(user,*lIt,sock);
		this->SendOk(sock);
	}else{
		this->SendFail(sock);
		syslog(LOG_NOTICE,"uuid not found");
	}
    transfermutex.Unlock();
}

void FtdApp::SendOk(UnixClientSocket* sock){
	
	Json::Value reply(Json::objectValue);
	reply["cmd"]=CMD_OK;
	this->SendJsonValue(reply,sock);
}

void FtdApp::SendFail(UnixClientSocket* sock){

	Json::Value reply(Json::objectValue);
	reply["cmd"]=CMD_FAIL;
	this->SendJsonValue(reply,sock);
}

void FtdApp::SendJsonValue(Json::Value& val, UnixClientSocket* sock){
	string r=writer.write(val);
	sock->Send(r.c_str(),r.length());
}

void FtdApp::AddFilter(DownloadManager* mgr){
	this->managers.push_back(mgr);
}

Downloader* FtdApp::FindDownloader(const URL &url){
	Downloader* dl=NULL;
	map<string,string> hints;
	list<DownloadManager*>::const_iterator lIt=this->managers.begin();
	while(lIt!=this->managers.end()){
		dl=(*lIt)->Filter(url,hints);
		if (dl!=NULL) {
			dl->SetHints(hints);
			break;
		}
		++lIt;
	}
	return dl;
}

void FtdApp::ShutDown(){

	const struct timespec delay = {0,100000000L};
    transfermutex.Lock();
	map<string,list<Downloader*> >::iterator tIt=this->transfers.begin();
    while(tIt!=this->transfers.end()){
    	syslog(LOG_DEBUG,"Stopping downloads for: %s",(*tIt).first.c_str());
        
        list<Downloader*>::iterator lIt=(*tIt).second.begin();
        while(lIt!=(*tIt).second.end()){
        	syslog(LOG_DEBUG,"\tstopping : %s",(*lIt)->GetUrl().c_str());
        	(*lIt)->StopDownload();
        	// Cant use Wait here since we cant keep atomic operation, we
        	// might hang between check if ok to wait and actual wait
        	// TODO: find better solution.
        	//(*lIt)->WaitForCompletion();
			// Make sure downloader really is done
        	// TODO: move this into waitforcomplertion
			while(!(*lIt)->ReapOK()){
						nanosleep(&delay, NULL);
			}
        	delete *lIt;
		++lIt;
        }
        ++tIt;
    }
    this->transfers.clear();
    transfermutex.Unlock();
}

FtdApp::FtdApp(){
	// Make sure curl is initialized
	// Uggly hack
	CurlDownloadManager::Instance();
}

FtdApp& FtdApp::Instance(void){
	static FtdApp instance;
	return instance;
}

FtdApp::~FtdApp(){
}
