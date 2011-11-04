/*

    DownloadManager - http://www.excito.com/

    SocketFdDownloader.cpp - this file is part of DownloadManager.

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

    $Id: SocketFdDownloader.cpp 2132 2008-11-12 15:38:30Z tor $
*/

#include "SocketFdDownloader.h"
#include <libeutils/Socket.h>
#include <libeutils/FileUtils.h>
#include <libeutils/StringTools.h>
#include <libeutils/UserGroups.h>
#include <iosfwd>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif


void SocketFdDownloader::StartDownload(){
	this->status=QUEUED;
	m_Thread = boost::thread(&SocketFdDownloader::Run, this);
}

void SocketFdDownloader::FilterFields(const string& field){

	if(field=="UUID"){
		this->uuid=this->cgi.field(field);
	}else if(field=="UPLOADPATH"){
		// Verify field
		if(this->cgi.field(field)!=""){
			this->destinationpath=StringTools::UrlDecode(this->cgi.field(field));
		}else{
			// Bail out?
		}
		if(!FileUtils::IsWritable(this->destinationpath.c_str(),User::UIDToUser(this->GetUser()).c_str())){
			this->CancelDownload();
		}
	}
}

void SocketFdDownloader::Run(){

	char buf[128];
	char boundary[128];
	bool err=false;
	UnixClientSocket sock(SOCK_STREAM,this->url.Path());

	try{

		int retries=10;
		bool connected;
		while(!(connected=sock.Connect()) && retries--){
			syslog(LOG_DEBUG,"Connect failed: %m");
			sleep(1);
		}

		if(!connected){
			syslog(LOG_ERR,"Unable to connect to socket: %m");
			err=true;
		}else{

			this->fd=sock.ReceiveFd();

			sock.Send("!",1);

			size_t bytes_read=sock.Receive(boundary,sizeof(boundary));
			boundary[bytes_read]='\0';

			sock.Send("!",1);

			sock.Receive(buf,sizeof(buf));

			long long content_length=atoll(buf);
			sock.Send("!",1);

			if( 1 /* setegid(this->group)==0*/ ){
				syslog(LOG_DEBUG,"Group changed");
				if(1 /*seteuid(this->user)==0*/){
					syslog(LOG_DEBUG,"UID changed");
					cgi.setFileDescriptor(this->fd);
					cgi.setBoundary(boundary);
					cgi.setContentLength(content_length);
					cgi.setIsMulti(true);
					string uldir="/home/"+User::UIDToUser(this->user)+"/.uploads";
					if(!Stat::DirExists(uldir)){
						FileUtils::MkDir(uldir);
						FileUtils::Chown(uldir,this->user,this->group);
					}
					cgi.setUploadDirectory(uldir);

					cgi.FieldAdded.connect(sigc::mem_fun(this,&SocketFdDownloader::FilterFields));

					cgi.parse();

					if(cgi.field("UPLOADPATH")==""){
					}else{
						this->destinationpath=StringTools::UrlDecode(cgi.field("UPLOADPATH"));
					}

				}else{
					syslog(LOG_ERR,"Error setting group (%m)");
					err=true;
				}
			}else{
				syslog(LOG_ERR,"Error setting user(%m)");
				err=true;
			}

		}
	}catch(runtime_error* e){
		syslog(LOG_ERR, "Caught exception: %s",e->what() );
		err=true;
	}

	sprintf(buf,"Im done");
	sock.Send(buf,strlen(buf)+1);

	try{
		if(err || this->status==CANCELINPROGRESS){
			this->CleanUp();
			if (this->status==CANCELINPROGRESS) {
				this->status=CANCELDONE;
				this->SignalFailed.emit("Upload Canceled");
			}else{
				this->status=FAILED;
				this->SignalFailed.emit("Upload failed");
			}
		}else{
			this->status=DOWNLOADED;
			this->FixFiles();
			this->SignalDone.emit();
		}
	}catch(runtime_error* e){
		syslog(LOG_ERR, "Caught exception: %s",e->what());
		err=true;
	}

	// Notify any watchers that we are done.
	this->Complete();

}

void SocketFdDownloader::CleanUp(){
	// Remove tempfiles
	map<string,string> filenames=cgi.fileNames();
	for(map<string,string>::iterator lIt=filenames.begin();lIt!=filenames.end();++lIt){
		unlink((*lIt).first.c_str());
	}

	// Remove socket
	unlink(this->url.Path().c_str());
}

void SocketFdDownloader::FixFiles(){
	if ( ((this->user!=0xffff) || (this->group!=0xffff))) {
		mode_t extraperm=0;

		const string web="/home/web";
		const string stor="/home/storage";

		// Fix paths for public areas.
		if(this->destinationpath.substr(0,web.length())==web){
			// Anyone can read from web folder. But not write
			extraperm=S_IROTH;
		}

		if(this->destinationpath.substr(0,stor.length())==stor){
			// Storage is public to all.
			extraperm=S_IROTH|S_IWOTH;
		}

		// Do we have writeaccess to destination
		if(!FileUtils::IsWritable(this->destinationpath.c_str(),User::UIDToUser(this->GetUser()).c_str())){
			syslog(LOG_ERR,"Destination %s not writeable by user %s"
				,this->destinationpath.c_str()
				,User::UIDToUser(this->GetUser()).c_str());
			this->CleanUp();
			return;
		}

		map<string,string> filenames=cgi.fileNames();

		// Start by changing ownership
		for(map<string,string>::iterator lIt=filenames.begin();lIt!=filenames.end();++lIt){
			if ( chown(
					(*lIt).first.c_str(),
					this->user==0xffff?0:this->user,
					this->group==0xffff?0:this->group) ) {
				syslog(LOG_ERR,"Chown failed: %s %m",(*lIt).first.c_str());
			}
			// Chmod
			if(chmod((*lIt).first.c_str(),S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|extraperm)){
				syslog(LOG_ERR,"Chmod failed: %s %m",(*lIt).first.c_str());
			}

		}

		// Should we move or copy files?
		dev_t srcdev=Stat(this->cgi.getUploadDirectory()).GetDevice();
		dev_t dstdev=Stat(this->destinationpath).GetDevice();
		if(srcdev==dstdev){
			// Do move
			syslog(LOG_DEBUG,"Upload: Move files");
			for(map<string,string>::iterator lIt=filenames.begin();lIt!=filenames.end();++lIt){
				string dest=this->destinationpath+"/"+(*lIt).second;
				// Move into right place.
				if(rename((*lIt).first.c_str(),dest.c_str())){
					syslog(LOG_ERR,"Unable to move file to destination: %m");
					if(unlink((*lIt).first.c_str())){
						string err=string("Unable remove original file: ")+strerror(errno);
						throw new runtime_error(err);
					}
				}
			}
		}else{
			syslog(LOG_DEBUG,"Upload: Copy and delete files");
			// Do copy
			for(map<string,string>::iterator lIt=filenames.begin();lIt!=filenames.end();++lIt){
				try{
					string dest=this->destinationpath+"/"+(*lIt).second;
					FileUtils::CopyFile((*lIt).first.c_str(),dest.c_str());
					//TODO: do we have to do this?
					if ( chown(dest.c_str(),this->user==0xffff?0:this->user,this->group==0xffff?0:this->group) ) {
						syslog(LOG_ERR,"Chown failed: %s %m",dest.c_str());
					}
				}catch(runtime_error &e){
					syslog(LOG_ERR,"Unable to copy file to destination: %s",e.what());
				}
				// Remove src file
				if(unlink((*lIt).first.c_str())){
					string err=string("Unable remove original file: ")+strerror(errno);
					throw new runtime_error(err);
				}
			}
		}
	}

}

void SocketFdDownloader::CancelDownload(void){

	syslog(LOG_DEBUG,"Cancel upload");

	if (this->status!=CANCELDONE && this->status!=DOWNLOADED && this->status!=FAILED) {

		this->status=CANCELINPROGRESS;

		cgi.cancelUpload();

	}
}

void SocketFdDownloader::StopDownload(void){
	syslog(LOG_DEBUG,"Stop upload");
	this->CancelDownload();
}

SocketFdDownloader::~SocketFdDownloader(){
}

SocketFdDownloadManager& SocketFdDownloadManager::Instance(){
	static SocketFdDownloadManager mgr;

	return mgr;
}

SocketFdDownloadManager::SocketFdDownloadManager(){
}


SocketFdDownloader* SocketFdDownloadManager::NewSocketFdDownloader(){

	return new SocketFdDownloader::SocketFdDownloader(this);
}

Downloader* SocketFdDownloadManager::Filter(const URL& url,map<string,string>& hints){
	Downloader* dl=NULL;
	if (url.Scheme()=="upload") {
		dl=this->NewSocketFdDownloader();
	}
	return dl;
}


