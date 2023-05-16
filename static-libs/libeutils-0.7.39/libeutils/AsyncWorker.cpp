/*

    libeutils - http://www.excito.com/

    AsyncWorker.cpp - this file is part of libeutils.

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

#include "AsyncWorker.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

static void sighand(int signo){
    printf("Caught signal %d\n",signo);
}

namespace EUtils{

AsyncWorker::AsyncWorker(){

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = sighand;
    sigaction(SIGALRM,&actions,NULL);
    this->running=true;

    Start();
    // Give worker thread time to start.
    this->Yield();

}

AsyncWorker::~AsyncWorker(){
	Flush();
    this->running=false;
    this->Cancel();
}

void AsyncWorker::AddLast(void* buf){

    listlock.Lock();
    write_buffer.push_back(buf);
    listlock.Unlock();

    NotifyWorker();

}

void AsyncWorker::AddFirst(void* buf){
    listlock.Lock();

    write_buffer.push_front(buf);

    listlock.Unlock();

    NotifyWorker();
}


void* AsyncWorker::Get() {
    void* buf;

    listlock.Lock();

    if (!write_buffer.empty()) {
        buf=write_buffer.front();
        write_buffer.pop_front();
    } else {
        buf=NULL;
    }
    listlock.Unlock();

    return buf;

}

int AsyncWorker::Size(){
    int ret=0;

    listlock.Lock();

    ret=this->write_buffer.size();

    listlock.Unlock();

    return ret;
}

void AsyncWorker::Cancel(){

    Signal(SIGALRM);

}

void AsyncWorker::NotifyWorker(){

    data_available.Notify();

}

void AsyncWorker::Flush(){
    void* buf;

    listlock.Lock();

    while (!write_buffer.empty()) {
        buf=write_buffer.front();
        write_buffer.pop_front();
        free(buf);
    }

    listlock.Unlock();
}


void AsyncWorker::Run(){

    void* buf;

    while (this->running) {

        // Wait for data
    	if(this->Size()==0){
    		data_available.Wait();
    	}

        while ( (buf=Get())!=NULL ) {
            this->ProcessElem(buf);

            free(buf);
        }

    }

}

void AsyncWorker::ProcessElem(void* buf){

    printf("Process buffer...\n");

}
}
