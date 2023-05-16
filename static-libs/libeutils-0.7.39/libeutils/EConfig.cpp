/*

    libeutils - http://www.excito.com/

    EConfig.cpp - this file is part of libeutils.

    Copyright (C) 2007 Tor Krill <tor@excito.com>

    libeutils is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    libeutils is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with libeutils; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/


#include "EConfig.h"
#include "StringTools.h"
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstring>

namespace EUtils{

static bool load_file(const char* name, char** buf, ssize_t* size);
static bool save_file(const char* name, char* buf, ssize_t size);

int EConfig::usecount=0;
list<EConfig*>* EConfig::cfglist;
Mutex* EConfig::listlock;

void EConfig::SigHandler(int signal){
	//std::cout << "got signal: "<<signal<<endl;
	list<EConfig*>::iterator lIt;
	EConfig::listlock->Lock();
	for(lIt=EConfig::cfglist->begin();lIt!=EConfig::cfglist->end();lIt++){
		//std::cout << "Calling update on config object"<<std::endl;
		(*lIt)->UpdateFromDisk(signal==SIGHUP);
	}
	EConfig::listlock->Unlock();
}

EConfig::EConfig(string cfgpath){

	this->cfg_path=StringTools::GetPath(cfgpath);
	this->cfg_file=cfgpath;

	if(stat(this->cfg_file.c_str(),&this->sbuf)<0){
		throw std::runtime_error("Failed to stat config file");
	}
	this->last_stat=time(NULL);
	this->stat_positive=true;

	if(EConfig::usecount++==0){
		//std::cout << "First instance setup static vars"<<std::endl;
		EConfig::cfglist=new list<EConfig*>();
		EConfig::listlock=new Mutex();
		if(signal(SIGRTMIN+3,EConfig::SigHandler)==SIG_ERR){
			throw std::runtime_error(string("Unable to attach to sigrtmin+3: ")+strerror(errno));
		}
		if(signal(SIGHUP,EConfig::SigHandler)==SIG_ERR){
			throw std::runtime_error(string("Unable to attach to sighup")+strerror(errno));
		}
	}

	if(!(this->cfg=g_key_file_new())){
		throw std::runtime_error("Falied to create config");
	}

	ssize_t size;
	char* buf;
	if(!load_file(this->cfg_file.c_str(),&buf,&size)){
		throw std::runtime_error("Unable to load config from file");
	}

	if(!g_key_file_load_from_data(this->cfg,buf,size,G_KEY_FILE_NONE,NULL)){
		throw std::runtime_error("Unable to parse config from file");
	}
	free(buf);

	if((this->dnot_fd=open(this->cfg_path.c_str(),O_ASYNC))<0){
		throw std::runtime_error(string("Unable to open dir for watch ")+strerror(errno));
	}

	if(fcntl(this->dnot_fd,F_SETSIG,SIGRTMIN+3)<0){
		throw std::runtime_error(string("Failed to alter signal")+strerror(errno));
	}

	if(fcntl(this->dnot_fd,F_NOTIFY,DN_MODIFY|DN_MULTISHOT)<0){
		throw std::runtime_error(string("Failed to add dnotify")+strerror(errno));
	}


	// Add ourself on notify list.
	EConfig::listlock->Lock();
	EConfig::cfglist->push_back(this);
	EConfig::listlock->Unlock();
}

bool EConfig::HasGroup(const string& group){
	bool res;
	this->cfg_mutex.Lock();
	res=g_key_file_has_group(this->cfg,group.c_str());
	this->cfg_mutex.Unlock();
	return res;
}

bool EConfig::HasKey(const string& group,const string& key){
	bool res;
	this->cfg_mutex.Lock();
	res=g_key_file_has_key(this->cfg,group.c_str(),key.c_str(),NULL);
	this->cfg_mutex.Unlock();
	return res;
}

string EConfig::GetValue(const string& group,const string& key){
	string ret;
	this->cfg_mutex.Lock();
	gchar* val=g_key_file_get_value(this->cfg,group.c_str(),key.c_str(),NULL);
	this->cfg_mutex.Unlock();
	if(val){
		ret=val;
		g_free(val);
		return ret;
	}else{
		throw std::runtime_error("Unable to find value");
	}
}

string EConfig::GetString(const string& group,const string& key){
	string ret;
	this->cfg_mutex.Lock();
	gchar* val=g_key_file_get_string(this->cfg,group.c_str(),key.c_str(),NULL);
	this->cfg_mutex.Unlock();
	if(val){
		ret=val;
		g_free(val);
		return ret;
	}else{
		throw std::runtime_error("Unable to find string");
	}
}

bool EConfig::GetBool(const string& group,const string& key){
	GError* err=NULL;
	this->cfg_mutex.Lock();
	gboolean bo=g_key_file_get_boolean(this->cfg,group.c_str(),key.c_str(),&err);
	this->cfg_mutex.Unlock();

	if(err==NULL){
		return bo;
	}else{
		throw std::runtime_error("Unable to find bool value");
	}
}

int EConfig::GetInteger(const string& group,const string& key){
	GError* err=NULL;
	this->cfg_mutex.Lock();
	gint in=g_key_file_get_integer(this->cfg,group.c_str(),key.c_str(),&err);
	this->cfg_mutex.Unlock();

	if(err==NULL){
		return in;
	}else{
		throw std::runtime_error("Unable to find integer value");
	}
}

#if (GLIB_CHECK_VERSION(2,12,0))

double EConfig::GetDouble(const string& group,const string& key){
	GError* err=NULL;
	this->cfg_mutex.Lock();
	gdouble db=g_key_file_get_double(this->cfg,group.c_str(),key.c_str(),&err);
	this->cfg_mutex.Unlock();

	if(err==NULL){
		return db;
	}else{
		throw std::runtime_error("Unable to find double value");
	}
}

void EConfig::SetDouble(const string& group,const string& key,const double value){

	this->cfg_mutex.Lock();
	g_key_file_set_double(this->cfg,group.c_str(),key.c_str(),value);
	this->WriteConfig();
	this->cfg_mutex.Unlock();

}


#endif

void EConfig::SetValue(const string& group,const string& key,const string& value){

	this->cfg_mutex.Lock();
	g_key_file_set_value(this->cfg,group.c_str(),key.c_str(),value.c_str());
	this->WriteConfig();
	this->cfg_mutex.Unlock();

}

void EConfig::SetString(const string& group,const string& key,const string& value){

	this->cfg_mutex.Lock();
	g_key_file_set_string(this->cfg,group.c_str(),key.c_str(),value.c_str());
	this->WriteConfig();
	this->cfg_mutex.Unlock();
}

void EConfig::SetBool(const string& group,const string& key,const bool value){

	this->cfg_mutex.Lock();
	g_key_file_set_boolean(this->cfg,group.c_str(),key.c_str(),value);
	this->WriteConfig();
	this->cfg_mutex.Unlock();

}

void EConfig::SetInteger(const string& group,const string& key,const int value){

	this->cfg_mutex.Lock();
	g_key_file_set_integer(this->cfg,group.c_str(),key.c_str(),value);
	this->WriteConfig();
	this->cfg_mutex.Unlock();

}




void EConfig::WriteConfig(void){
	gsize sz;
	this->file_mutex.Lock();

	this->selfwrite=true;

	gchar* file=g_key_file_to_data (this->cfg,&sz,NULL);

	if(file){
		// File should be up to date if we wrote it ourself
		this->stat_positive=true;
		if(!save_file(this->cfg_file.c_str(),file,sz)){
			g_free(file);
			this->selfwrite=false;
			this->file_mutex.Unlock();
			// We now that the callee wont catch this exception so we release the lock for them
			this->cfg_mutex.Unlock();
			throw std::runtime_error("Unable to write config file");
		}
		g_free(file);
	}else{
		this->selfwrite=false;
		this->file_mutex.Unlock();
		throw std::runtime_error("Unable to convert config to string");
	}
	this->selfwrite=false;
	this->file_mutex.Unlock();

}

static bool save_file(const char* name, char* buf, ssize_t size){
	int fd;
	int res;
	ssize_t wr,wr_tot=0;

	if((fd=open(name,O_WRONLY|O_CREAT|O_TRUNC,00660))<0){
		return false;
	}

	do{
		res=flock(fd,LOCK_EX);
	}while(res==EINTR);

	if(res!=0){
		close(fd);
		return false;
	}

	// Save file
	// Important, we rely on lazy evaluation here. IE that write does not
	// Occur if wr_tot>=size
	while(size>wr_tot && (wr=write(fd,buf+wr_tot,size-wr_tot))>0){
		wr_tot+=wr;
	}

	do{
		res=flock(fd,LOCK_UN);
	}while(res==EINTR);


	close(fd);
	return true;

}

static bool load_file(const char* name, char** buf, ssize_t* size){
	struct stat sbuf;
	int fd;
	int res;
	ssize_t rd,rd_tot=0;

	if(stat(name,&sbuf)<0){
		return false;
	}

	if(sbuf.st_size==0){
		return false;
	}

	if((fd=open(name,O_RDONLY))<0){
		return false;
	}

	do{
		res=flock(fd,LOCK_SH);
	}while(res==EINTR);

	if(res!=0){
		close(fd);
		return false;
	}

	if((*buf=(char*)malloc(sbuf.st_size))==NULL){
		close(fd);
		return false;
	}

	while((rd=read(fd,*buf+rd_tot,sbuf.st_size-rd_tot))>0){
		rd_tot+=rd;
	}

	if(rd<0){
		close(fd);
		free(*buf);
		return false;
	}


	do{
		res=flock(fd,LOCK_UN);
	}while(res==EINTR);

	*size=rd_tot;
	close(fd);
	return true;
}


// This method is a bit tricky. Can't use exceptions here since our destructor could be running
// resulting in a nice deadlock.

void EConfig::UpdateFromDisk(bool force){
	struct stat l_stat;
	GError* err=NULL;
	time_t now;

	if(this->selfwrite){
		return;
	}

	/* Only do one update per second */
	if(((now=time(NULL))==this->last_stat)&&this->stat_positive){
		//std::cout << "Skipping update"<<std::endl;
		return;
	}else{
		this->last_stat=now;
	}

	this->file_mutex.Lock();

	if(stat(this->cfg_file.c_str(),&l_stat)<0){
		// Give it a second try
		usleep(10000);
		if(stat(this->cfg_file.c_str(),&l_stat)<0){
			this->file_mutex.Unlock();
			//std::cerr << "Stat of config file failed"<<std::endl;
			return;
		}
	}


	if((l_stat.st_ctime!=this->sbuf.st_ctime)||force){

		// File modified timestamp changed
		if(!force){
			this->stat_positive=true;
		}

		g_key_file_free(this->cfg);

		if(!(this->cfg=g_key_file_new())){
			this->file_mutex.Unlock();
			//std::cerr << "Failed to create config"<<std::endl;
			return;
		}

		ssize_t size;
		char* buf;
		if(!load_file(this->cfg_file.c_str(),&buf,&size)){
			// Try once more
			usleep(100000);
			if(!load_file(this->cfg_file.c_str(),&buf,&size)){
				this->file_mutex.Unlock();
				//std::cerr <<"Unable to load config from file"
				//	<<this->cfg_file<<std::endl;
				return;
			}
		}
		if(size>0){
			if(!g_key_file_load_from_data(this->cfg,buf,size,G_KEY_FILE_NONE,&err)){
					free(buf);
					this->file_mutex.Unlock();
					//std::cerr <<"Unable to parse config from data:"
					//	<<" cause:"<<err->message<<std::endl;
					return;
			}
		}

		free(buf);

	}else{

		// No change to file
		this->stat_positive=false;
	}
	this->file_mutex.Unlock();


}

EConfig::~EConfig(void){
	EConfig::listlock->Lock();
	EConfig::cfglist->remove(this);

	close(this->dnot_fd);

	if(--EConfig::usecount==0){
		EConfig::listlock->Unlock();
		//std::cout<<"Last element removed"<<endl;
		delete EConfig::cfglist;
		delete EConfig::listlock;
	}else{
		EConfig::listlock->Unlock();
	}

	g_key_file_free(this->cfg);
}
}
