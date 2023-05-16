/*

    libeutils - http://www.excito.com/

    Mutex.cpp - this file is part of libeutils.

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

#include "Mutex.h"
#include <stdexcept>

namespace EUtils{

Mutex::Mutex(int type){
	if(type){
		pthread_mutexattr_t   mta;
		int rc = pthread_mutexattr_init(&mta);
		if(rc){
			throw new std::runtime_error("Unable to create mutex attributes");
		}

		if(type==MUTEX_RECURSIVE){
			if(pthread_mutexattr_settype(&mta,PTHREAD_MUTEX_RECURSIVE)){
				throw new std::runtime_error("Unable to set recursive attribute on mutex");
			}
		}else if(type==MUTEX_ERRORCHECK){
			if(pthread_mutexattr_settype(&mta,PTHREAD_MUTEX_ERRORCHECK)){
				throw new std::runtime_error("Unable to set recursive attribute on mutex");
			}
		}else{
			throw new std::runtime_error("Illegal mutex type");
		}

		pthread_mutex_init(&lock,&mta);

		if(pthread_mutexattr_destroy(&mta)){
			throw new std::runtime_error("Failed to destroy mutex init structure");
		}

	}else{
    	pthread_mutex_init(&lock,NULL);
	}
}

void Mutex::Lock(){
    pthread_mutex_lock(&lock);
}

void Mutex::Unlock(){
    pthread_mutex_unlock(&lock);
}

pthread_mutex_t* Mutex::GetPThreadMutex(){
    return &lock;
}

Mutex::~Mutex(){
}

}
