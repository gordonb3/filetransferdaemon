/*
    
    DownloadManager - http://www.excito.com/
    
    DirWatcher.cpp - this file is part of DownloadManager.
    
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
    
    $Id: DirWatcher.cpp 2144 2008-11-17 21:02:28Z tor $
*/

#include <stdexcept>
#include <iostream>
#include <errno.h>
#include <syslog.h>

#include <sys/inotify.h>

#include "FtdConfig.h"
#include <libeutils/FileUtils.h>
#include <libeutils/StringTools.h>
#include <libeutils/UserGroups.h>
#include <libeutils/EExcept.h>
#include "DirWatcher.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


DirWatcher::DirWatcher(){
	this->mngr=&TorrentDownloadManager::Instance();
	this->app=&FtdApp::Instance();
	this->doRun=true;

	// Build base glob pattern
	string wp="/home/*";
	string suf="/torrents/";
	try{
		FtdConfig& cfg=FtdConfig::Instance();
		suf="/"+cfg.GetString("torrent","torrentdir")+"/";
	}catch(std::runtime_error& err){
		syslog(LOG_INFO,"Unable to read torrent dir, using default");
	}
	this->baseglob=wp+suf;
	
	if((this->inotifyfd=inotify_init())<0){
		syslog(LOG_ERR,"Failed to init inotify because %m");
	}	

}

DirWatcher::~DirWatcher(){
	if(this->inotifyfd>=0){
		close(this->inotifyfd);
	}

}
void DirWatcher::Start() {
	m_Thread = boost::thread(&DirWatcher::Run, this);
}

void DirWatcher::Run(){
	// Load torrents already in place
	this->LoadInitial();
	
	if(this->inotifyfd>=0){
		char buf[1024];
		// Add watches on all current directories
		this->AddWatches();
		
		// Monitor /home for new and removed users
		this->AddHomeWatch();

		// TODO: implement timeout and a way to terminate thread
		ssize_t l;
		struct inotify_event *item;	
		int itemsize;
		while(doRun){
			if((l=read(this->inotifyfd,buf,sizeof(buf)))<0){
				if(errno != EINTR){
					syslog(LOG_NOTICE,"Read event failed: %m");
				}
			}else{
				item=(inotify_event *)buf;
		
				while(l>=(ssize_t)(sizeof(struct inotify_event)+item->len)){
					itemsize=(sizeof(struct inotify_event)+item->len);
					if(item->len ){
						if(this->iwatches[item->wd]=="/home/"){
							syslog(LOG_DEBUG,"dirwatcher: user %s trigged",item->name);
							// This is an action on the home watch
							string pth="/home/"+string(item->name)+"/"+
									FtdConfig::Instance().GetStringOrDefault("torrent","torrentdir","torrents")+
									"/";
							if(item->mask & IN_CREATE){
								// New user
								// Hack to let system add skeleton dir
								sleep(1);
								this->AddWatch(pth);
							}else if(item->mask & IN_DELETE){
								// User deleted, do nothing. Watch removed when torrent dir deleted
								//this->DelWatch(pth);
							}else{
								syslog(LOG_NOTICE, "Unknown event on home dir: %s",pth.c_str());
							}
						}else{
							// TODO: add watch on delete as well.
							// This is an ordinary action on a torrent dir
							if(item->mask && (IN_CREATE|IN_MOVED_TO)){
								// We have a new file, only add if its not our "own"
								if(strncmp(item->name,".ftdtr",6)==0){
									syslog(LOG_DEBUG,"Not readding download");
								}else{
									string dlpath=this->iwatches[item->wd]+string(item->name);
									//AFP hack
									sleep(1);
									this->AddDownload(dlpath);
								}
							}else{
								syslog(LOG_NOTICE,"dirwatcher: Unknown event on %s",item->name);
							}
						}
					}
					l-=itemsize;
					item=(struct inotify_event*)(((unsigned long)item)+itemsize);
				}
			}			
		}
	}			
	syslog(LOG_NOTICE,"Dirwatcher terminating");
}

void DirWatcher::DownloadFailed(const string& err,void* d){
	TorrentDownloader* dl=static_cast<TorrentDownloader*>(d);
	syslog(LOG_INFO,"Download of [%s] failed because: [%s]",dl->GetUrl().c_str(),err.c_str());

	Json::Value rep(Json::objectValue);
	rep["user"]=User::UIDToUser(dl->GetUser());
	rep["url"]=dl->GetUrl();
	rep["uuid"]=dl->GetUUID();
	this->app->CancelDownload(rep);
}

void DirWatcher::AddDownload(const string& path){
	try{
		Stat st(path);
		string dpath;
		if(S_ISDIR(st.GetMode())){
			syslog(LOG_INFO,"Path is directory, not adding");
			return;
		}
		
		// Avoid adding AFP litter files :(
		// Todo: Make something more generic.
		if(path.size()>11 && (path.substr(path.size()-11)==":2eDS_Store")){
			syslog(LOG_DEBUG,"Skip AFP index file");
			return;
		}

		try{
			dpath=this->app->GetDownloadDir(st.GetOwner());
		}catch(std::runtime_error& err){
			syslog(LOG_ERR,"Unable to get homedir of user, not adding download");
			return;
		}
		
		TorrentDownloader* down=mngr->NewTorrentDownloader();
		
		down->SetUser(st.GetUID());
		down->SetGroup(Group::GroupToGID("users"));
		down->SetUUID(StringTools::SimpleUUID());
		down->SetPolicy(DLP_NONE);
		down->SetDestinationPath(dpath);
		down->SetUrl("file://"+path);
		down->SetTorrentFilename(path);

		if(down->StartFromFile(path)){

			down->SignalFailed.connect(sigc::bind(sigc::mem_fun(this,&DirWatcher::DownloadFailed),down));
			app->RegisterDownload(st.GetOwner(),down);

		}else{
			syslog(LOG_NOTICE,"Failed to add download from file");

			// Dont delete torrent since we cant be sure its state at all
			down->SetDeletestatus(false);

			delete down;
		}

	}catch(EExcept::ENoent &err){
		syslog(LOG_ERR,"Torrent file does not exist: %s",err.what());
	}
}

void DirWatcher::Stop(){
	this->doRun=false;
}

void DirWatcher::LoadInitial(){
	
	// Glob for generated torrentfiels
	string gp=this->baseglob+".ftd*";
	list<string> files;

	try{
		files=FileUtils::Glob(gp);
	}catch(EExcept::ENoent &err){
		syslog(LOG_DEBUG,"Glob saved system torrents: no matches");
	} 

	//Glob for user added files
	gp=this->baseglob+"*";

	try{
		list<string> userfiles=FileUtils::Glob(gp);
		files.splice(files.end(),userfiles);
	}catch(EExcept::ENoent &err){
		syslog(LOG_DEBUG,"Glob useradded torrents: no matches");
	} 

	for(list<string>::iterator lIt=files.begin();lIt!=files.end();++lIt){
		//std::cerr << "File: ["<<*lIt<<"]"<<std::endl;
		this->AddDownload(*lIt);
	}
	
}
/* Add the watch on home directory used to monitor new user additions*/
void DirWatcher::AddHomeWatch(void){
	int w;
	if((w=inotify_add_watch(this->inotifyfd,"/home/",IN_CREATE|IN_DELETE))<0){
		syslog(LOG_ERR,"Failed to add watch on /home: %m");
	}else{
		this->iwatches[w]="/home/";
	}		
}

void DirWatcher::AddWatch(const string& path){
	int w;
	syslog(LOG_DEBUG,"Add watch on: [%s]",path.c_str());
	if((w=inotify_add_watch(this->inotifyfd,path.c_str(),IN_CLOSE_WRITE|IN_MOVED_TO))<0){
		syslog(LOG_ERR,"Failed to add watch on %s: %m",path.c_str());
	}else{
		this->iwatches[w]=path;
	}		
}

void DirWatcher::DelWatch(const string& path){
	map<int,string>::iterator mIt=this->iwatches.begin();
	while(mIt!=this->iwatches.end() && (*mIt).second!=path){
		++mIt;
	}
	if(mIt!=this->iwatches.end()){
		if(inotify_rm_watch(this->inotifyfd,(*mIt).first)<0){
			syslog(LOG_ERR,"Failed to remove watch: %m");
		}else{
			syslog(LOG_DEBUG,"Removed watch for %s",path.c_str());
			this->iwatches.erase(mIt);
		}
	}else{
		syslog(LOG_INFO,"Failed to locate watch to remove");
	}
}

void DirWatcher::AddWatches(){
	// Add watches in all matching torrent dirs
	try{
		list<string> files=FileUtils::Glob(this->baseglob);
		for(list<string>::iterator lIt=files.begin();lIt!=files.end();++lIt){
			this->AddWatch(*lIt);
		}
	}catch(EExcept::ENoent &err){
		syslog(LOG_INFO,"No matches, no torrent dir found");
	} 
}

DirWatcher& DirWatcher::Instance(void){
	static DirWatcher watcher;
	
	return watcher;
}
	
