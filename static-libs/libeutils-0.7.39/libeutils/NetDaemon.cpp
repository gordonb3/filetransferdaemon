/*

    libeutils - http://www.excito.com/

    NetDaemon.cpp - this file is part of NetDaemon.

    Copyright (C) 2009 Tor Krill <tor@excito.com>

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

#include "NetDaemon.h"

#include <iostream>

namespace EUtils{

NetDaemon::NetDaemon(const string& sockpath, int timeout=0 ):
		serv(SOCK_STREAM,sockpath),timeout(timeout),shutdown(false)
		,pendingreq(0){
	this->serv.SetTimeout(timeout,0);
}

void NetDaemon::ShutDown(){
	this->shutdown=true;
}

void NetDaemon::increq(){
	this->reqmutex.Lock();
	this->pendingreq++;
	this->reqmutex.Unlock();
}

void NetDaemon::decreq(){
	this->reqmutex.Lock();
	if(this->pendingreq>0){
		this->pendingreq--;
	}
	this->reqmutex.Unlock();
}


void NetDaemon::Dispatch(UnixClientSocket* con){
	char recbuf[1024];
	ssize_t r=con->Receive(recbuf, 1024);
	if(r>0){
		// Do nothing for now
	}
	this->decreq();
}

void NetDaemon::Run(){
	do{
		try{

			UnixClientSocket* con=this->serv.Accept();
			if(con){
				this->increq();
				this->Dispatch(con);
				delete con;
			}else{
				if(this->serv.TimedOut()){
					this->reqmutex.Lock();
					if(this->pendingreq==0){
						this->shutdown=true;
						syslog(LOG_NOTICE,"Server timed out, terminating");
					}else{
						syslog(LOG_NOTICE,"Timeout but got %d pending requests",this->pendingreq);
					}
					this->reqmutex.Unlock();
				}else{
					// Todo: Error
					syslog(LOG_ERR,"Failed on accepting connection");
				}
			}

		}catch(std::runtime_error& err){
			//Todo: error
			syslog(LOG_ERR,"Caught exception: %s",err.what());
		}

	}while(!shutdown);
	syslog(LOG_NOTICE,"Daemon terminating");

}


NetDaemon::~NetDaemon(){

}

}
