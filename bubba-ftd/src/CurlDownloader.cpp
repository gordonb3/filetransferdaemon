/*
    
    DownloadManager - http://www.excito.com/
    
    CurlDownloader.cpp - this file is part of DownloadManager.
    
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
    
    $Id: CurlDownloader.cpp 2015 2008-10-14 13:45:35Z tor $
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <sys/types.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>
#include <syslog.h>
#include <time.h>

#include "CurlDownloader.h"
#include <libeutils/StringTools.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif


using namespace std;

CurlDownloader::CurlDownloader(string url,string destination,string uuid):Downloader(url,destination,uuid),curl(NULL) {
	m_timeout=0;
	m_ctimeout=0;
	this->headersonly=false;
	this->m_useragent=USER_AGENT;
	this->mydest=true;
	this->dest=NULL;                
}

int CurlDownloader::CbProgress(void* client, double dltotal, 
						 double dlnow, double ultotal, double ulnow) {
	double speed;
	int ret;
	CurlDownloader* dl=static_cast<CurlDownloader*>(client);
	
	dl->curllock.Lock();

	if (dl->cancel) {
		dl->curllock.Unlock();
		return 0;
	}

	if(curl_easy_getinfo(dl->curl,CURLINFO_SPEED_DOWNLOAD,&speed)==CURLE_OK){
		Json::Value info(Json::objectValue);
		info["type"]="curl";
		info["speed"]=speed;
		dl->SetInfo(info);
	}

	ret=dl->UpdateProgress(dltotal,dlnow,ultotal,ulnow);

	dl->curllock.Unlock();
	
	return ret;
}

size_t CurlDownloader::WriteContent(void* data, size_t size, size_t nmemb, void* handle) {
	CurlDownloader* d=static_cast<CurlDownloader*>(handle);
	
	d->curllock.Lock();

	if (d->cancel) {
		d->curllock.Unlock();
		return 0;
	}

	if(d->dest==NULL){
		d->curllock.Unlock();
		return 0;
	}
	
	if(!d->dest->good()){
		d->curllock.Unlock();
		return 0;
	}

	if(!d->dest->write((const char*)data,size*nmemb).fail()){
		d->curllock.Unlock();
		return size*nmemb;
	}else{
		d->curllock.Unlock();
		return 0;
	}
}


size_t CurlDownloader::Headers(void *data, size_t size, size_t nmemb, void *handle) {
	CurlDownloader* d=static_cast<CurlDownloader*>(handle);

	d->curllock.Lock();

	if (d->cancel) {
		d->curllock.Unlock();
		return size*nmemb;
	}

	string s((char*)data,size*nmemb);

	int del=s.find(":");
	if ( del>=0 ) {
		string key=StringTools::ToLower(s.substr(0,del));
		string value=StringTools::Trimmed(s.substr(del+1)," \r\n");
		list<string> items=StringTools::Split(value,';');

		if ( items.size()>1 ) {
			// We have parameters add them and value
			list<string>::const_iterator lIt=items.begin();
			d->headers[key]=*(lIt++);
			//cerr << "Key1: ["<<key<<"] value ["<<d->headers[key]<<"]"<<endl; 
			for ( ;lIt!=items.end();++lIt ) {
				//cerr << "Parameter: ["<<*lIt<<"]"<<endl;
				list<string> par=StringTools::Split(*lIt,'=');
				if ( par.size()==2 ) {
					list<string>::const_iterator lPt=par.begin();
					d->headers[*lPt++]=*lPt++;
				} else {
					syslog(LOG_ERR,"Faulty parameter/value pair in header");
				}
			}
		} else if ( items.size()==1 ) {
			// We only have one value and no parameters.
			//cerr << "key: ["<<key<<"] value: ["<<value<<"]"<<endl;
			d->headers[key]=value;
		} else {
			// No value??
			//cerr<< "Got header without value:["<<key<<"]"<<endl;
		}
	}

	d->curllock.Unlock();
	
	return size*nmemb;
}

void CurlDownloader::SetStream(iostream *str){
	if (str && dest) {
		throw runtime_error("Stream already set");
	}
	if(str){
		this->mydest=false;
	}else{
		this->mydest=true;
	}
	dest=str;
}

void CurlDownloader::SetUserAgent(const string s){
	if (this->status==NOTSTARTED) {
		m_useragent=s;
	}
}

void CurlDownloader::SetHeadersOnly(bool which){
	this->headersonly=which;
}

map<string,string> CurlDownloader::GetHeaders(){
	return this->headers;
}


void CurlDownloader::StartDownload() {

	//cerr << "Start download: "<<this->url.GetUrl()<<endl;
	if ( this->url.GetUrl()=="" ) {
		throw std::runtime_error("No URL to download");
	}

	if ( this->destinationpath=="" && this->destinationname=="" ) {
		this->destinationname=this->url.GetUrl().substr(this->url.GetUrl().rfind("/")+1);
	}

	if ( !dest && !this->headersonly ) {
		this->dest=new fstream((this->destinationpath+"/"+this->destinationname).c_str(),ios_base::out|ios_base::trunc);
		this->mydest=true;
	}

	this->status=QUEUED;

	m_Thread = boost::thread(&CurlDownloader::Run, this);
}

void CurlDownloader::StopDownload(){
	this->CancelDownload();
}

void CurlDownloader::Run() {

	CURLcode res;

	this->curllock.Lock();
	curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, this->url.GetUrl().c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  CurlDownloader::WriteContent);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,CurlDownloader::CbProgress);

	if(this->headersonly){
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, this);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,CurlDownloader::Headers);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	}

	if (m_timeout>0) {
		if(m_ctimeout==0){
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60);
		}else{
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_ctimeout);
		}
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_timeout);
	}

	if (m_ctimeout>0 && m_timeout==0){
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_ctimeout);
	}		

	if(!m_useragent.empty()){
		curl_easy_setopt(curl, CURLOPT_USERAGENT, this->m_useragent.c_str());
	}

	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE,   1);	
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS,      5);
	this->curllock.Unlock();
	// We could have been canceled before we even have started
	// thus check once more if its ok to start
	if(this->status==QUEUED && !this->cancel){
		this->tip=true;
		this->status=INPROGRESS;
		syslog(LOG_DEBUG,"About to start download %p",this);
		res = curl_easy_perform(curl);
		// Check responsecode if acplicable
		long rescode;
		if(curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &rescode)==CURLE_OK){
			syslog(LOG_DEBUG,"Responsecode: %ld",rescode);
			if(this->url.Scheme()=="http"|| this->url.Scheme()=="https"){
				if(rescode>299){
					res=(CURLcode)10; // CURLE_OBSOLETE10 - reuse unused resultcodes
				}
			}
		}
		syslog(LOG_DEBUG,"Download done %p",this);
		//cerr << "Download done. Result: ["<<res<<"] url ["<< this->GetUrl()<<"]" <<endl;
		this->tip=false;
	}else{
		res=CURLE_OBSOLETE;
	}
	
	// Where transfer interrupted?
	if(this->cancel || this->status==CANCELINPROGRESS){
		this->status=CANCELDONE;
		// In this case signal ok. This is nothing extraordinary
	}else{
		if(res==0){
			// All went fine and we where not canceled

			// Make sure all content written to disk
			if(this->dest){
				syslog(LOG_DEBUG,"Flush stream, make sure all data is written");
				this->dest->flush();
			}
			
			// Try change rights
			if ( ((this->user!=0xffff) || (this->group!=0xffff))&& !this->headersonly ) {
				if ( chown((this->destinationpath+"/"+this->destinationname).c_str(),this->user==0xffff?0:this->user,this->group==0xffff?0:this->group) ) {
					syslog(LOG_NOTICE,"CurlDownloader: Chown failed");
				}
			}

			this->status=DOWNLOADED;

		}else if(res==CURLE_OBSOLETE){
			// We have not even done any downloading 
			// This is also ok
			this->status=CANCELDONE;

		}else{
			// We have an error
			this->status=FAILED;
		}
	}

	// Clean up.
	this->curllock.Lock();
	curl_easy_cleanup(curl);
	this->curllock.Unlock();


	if(this->status==DOWNLOADED){

		this->SignalDone.emit();
	
	}else if(this->status==CANCELDONE){
		this->SignalDone.emit();
		if (this->mydest) {
			unlink((this->destinationpath+"/"+this->destinationname).c_str());
		}
	}else{
		// Remove any possible leftovers
		if (this->mydest) {
			unlink((this->destinationpath+"/"+this->destinationname).c_str());
		}
		this->SignalFailed.emit(curl_easy_strerror(res));
	}
	
	// Tell everyone interrested that we are done
	this->Complete();
	
}

CurlDownloader::~CurlDownloader() {
	this->curllock.Lock();
	this->cancel=true;
	if (this->mydest && this->dest) {
		delete(dest);
	}
	this->curllock.Unlock();
}

CurlDownloadManager::CurlDownloadManager() {
	syslog(LOG_NOTICE,"Initializing curl");
	if(curl_global_init(CURL_GLOBAL_ALL)!=CURLE_OK){
		throw std::runtime_error("Failed to init curl");
	}
};


CurlDownloadManager& CurlDownloadManager::Instance(){
	static CurlDownloadManager mgr;
	return mgr;
}

CurlDownloader* CurlDownloadManager::GetDownloader(string url,string destination,string uuid){
	return new CurlDownloader(url,destination,uuid);
}

Downloader* CurlDownloadManager::Filter(const URL &url,map<string,string>& hints){
	const struct timespec delay = {0,100000000L};
	syslog(LOG_DEBUG,"Filter headers");
	if (url.Scheme()=="http" || url.Scheme()=="https" || url.Scheme()=="ftp") {
		if (url.Extension()=="torrent") {
			return NULL;
		}
		if (url.Scheme()=="http" || url.Scheme()=="https") {
			// TODO: use stringstream to not realy do any download when only interrrested in headers.
			CurlDownloader* dl=this->GetDownloader();
			dl->SetHeadersOnly(true);
			dl->SetConnectTimeout(5);
			dl->SetUrl(url.GetUrl());
			syslog(LOG_DEBUG,"Start downloading headers %p",dl);
			dl->StartDownload();
			syslog(LOG_DEBUG,"Wait for headers to be downloaded %p",dl);
			dl->WaitForCompletion();
			syslog(LOG_DEBUG,"Should be completed now %p",dl);
			if(dl->GetStatus()!=DOWNLOADED){
				syslog(LOG_DEBUG,"Download headers failed");
				// Make sure downloader really is done 
	        	// TODO: move this into waitforcomplertion
				while(!dl->ReapOK()){
							nanosleep(&delay, NULL);
				}
				delete(dl);
				return NULL;
			}
			hints=dl->GetHeaders();
			// Make sure downloader really is done 
        	// TODO: move this into waitforcomplertion
			while(!dl->ReapOK()){
						nanosleep(&delay, NULL);
			}
			syslog(LOG_DEBUG,"About to delete %p",dl);
			delete(dl);
			syslog(LOG_DEBUG,"Deleted %p",dl);
			if (hints["content-type"]=="application/x-bittorrent") {
				return NULL;
			}
		}
		return GetDownloader();
	}else{
		return NULL;
	}
}

bool CurlDownloadManager::ProvidesService(const string& service){
	return (service=="http" || service=="ftp");
}
