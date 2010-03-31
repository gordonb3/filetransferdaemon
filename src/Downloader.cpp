/*
    
    DownloadManager - http://www.excito.com/
    
    Downloader.cpp - this file is part of DownloadManager.
    
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
    
    $Id: Downloader.cpp 2015 2008-10-14 13:45:35Z tor $
*/

#include "Downloader.h"
#include <iostream>
#include <stdio.h>
#include <syslog.h>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif


Downloader::Downloader(string url,string dest, string name, string uuid){

    this->size=0;
    this->downloaded=0;
    if (!url.empty()) {
	    this->url.SetUrl(url);
    }
    this->destinationpath=dest;
    this->destinationname=name;
    this->tip=false;
    this->cancel=false;
    this->reapstatus=OK;
    this->status=NOTSTARTED;
    this->uuid=uuid;
    this->user=0xffff;
    this->group=0xffff;
    this->policy=0;
}


bool Downloader::UpdateProgress(double dltotal,double dlnow, 
                       double ultotal, double ulnow){

    this->size=dltotal;
    this->downloaded=dlnow;

    this->Progress();

    if (this->cancel) {
    	this->status=CANCELINPROGRESS;
        return 1;
    }else{
        return 0;
    }
}

void Downloader::SetUrl(string url){
    this->url.SetUrl(url);
}

string Downloader::GetUrl(){
    return this->url.GetUrl();
}


void Downloader::SetUser(uid_t user){
    this->user=user;
}

uid_t Downloader::GetUser(){
    return this->user;
}

void Downloader::SetGroup(gid_t group){
    this->group=group;
}

gid_t Downloader::GetGroup(){
    return this->group;
}


void Downloader::SetDestinationPath(string dest){
    this->destinationpath=dest;
}

string Downloader::GetDestinationPath(){
    return this->destinationpath;
}

void Downloader::SetDownloadName(string name){
	this->destinationname=name;
}

string Downloader::GetDownloadName(){
	return this->destinationname;
}

void Downloader::SetInfo(Json::Value info){
    this->info=info; 
}

Json::Value Downloader::GetInfo(){
	return this->info;
}

void Downloader::SetUUID(string uuid){
    this->uuid=uuid;
}
 
string Downloader::GetUUID(void){
    return this->uuid;
}

double Downloader::Size(){

    return this->size;
}

double Downloader::Downloaded(){
    return this->downloaded;
}

void Downloader::CancelDownload(){
    this->cancel=true;
}

void Downloader::StopDownload(){
	cerr << "Stop download, should be implemented in subclass"<<endl;
}

dlstatus Downloader::GetStatus(){
    return this->status;
}

void Downloader::WaitForCompletion(){

    if (!this->ReapOK()) {
        this->completion.Wait();
    }else{
    	syslog(LOG_DEBUG,"WaitForCompletion NOT waiting %p %d %d %d",this,this->reapstatus,this->status,this->cancel);
    }

}

bool Downloader::RegisterCompletionCallback(callback_t cb,void* data){
	bool ret=true;
	
	if(cb==NULL){
		syslog(LOG_ERR,"Trying to add null function as callback");
		return false;
	}
	
	this->cbmutex.Lock();
	if(this->reapstatus==OK){
		this->callbacks[cb].push_back(data);
	}else{
		ret=false;
	}
    this->cbmutex.Unlock();
    
    return ret;
}

void Downloader::InvokeCallbacks(){
	this->cbmutex.Lock();
	for (cb_map::iterator it=callbacks.begin();it!=callbacks.end();it++) {
		for (list<void*>::iterator lit=(*it).second.begin();
			lit!=(*it).second.end(); lit++) {

			((*it).first)((*lit));

		}
	}
	this->cbmutex.Unlock(); 
}

void Downloader::Complete(){
	bool proceed=false;
	
	this->reapmutex.Lock();
	if(this->reapstatus==OK){
		this->reapstatus=CALLINPROGRESS;
		proceed=true;
	}
	this->reapmutex.Unlock();
	
	if(!proceed){
		syslog(LOG_NOTICE,"Complete download called more than once");
		return;
	}
	
	this->completion.NotifyAll();
	this->InvokeCallbacks();
	
	this->reapmutex.Lock();
	this->reapstatus=CALLDONE;
	this->reapmutex.Unlock();
}

bool Downloader::ReapOK(){
	bool ret;
	this->reapmutex.Lock();
	
	ret=(this->reapstatus==CALLDONE)||(this->status==NOTSTARTED && this->cancel);

	this->reapmutex.Unlock();
	return ret;
}

void Downloader::Progress(){
    //printf("%s: %f\n",this->destination.c_str(),100.0*this->downloaded/this->size);
}

void Downloader::SetHints(map<string,string> const& hints){
	this->hints=hints;
}

Downloader::~Downloader(){
}

/* 
 * 
 * 	Download manager implementations 
 * 
 */
 
 bool DownloadManager::ProvidesService(const string& service){
 	return false;
 }
