/*

    libeutils - http://www.excito.com/

    NetCllient.cpp - this file is part of NetDaemon.

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

#include "NetClient.h"

#include <stdlib.h>
#include <unistd.h>


namespace EUtils{

NetClient::NetClient(const string& daemon, const string& path):
	UnixClientStreamSocket(SOCK_STREAM,path),daemon(daemon){

	if(!this->Connect()){
		this->SpawnDaemon();
		int retries=0;
		for(;retries<5;retries++){
			if(!this->Connect()){
				usleep(retries*retries*20000);
			}else{
				break;
			}
		}
		if(retries==5){
			throw std::runtime_error("Failed to connect to server");
		}
	}
}

void NetClient::SpawnDaemon(){
	if(system(daemon.c_str())<0){
		throw std::runtime_error("Failed to spawn daemon");
	}
}


NetClient::~NetClient(){

}

}
