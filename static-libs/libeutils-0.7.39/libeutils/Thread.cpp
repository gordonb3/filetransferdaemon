/*

    libeutils - http://www.excito.com/

    Thread.cpp - this file is part of libeutils.

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

#include "Thread.h"
#include <sys/signal.h>

namespace EUtils{

Thread::Thread(){

}

void* Thread::BootstrapThread(void* obj){
    Thread* q=static_cast<Thread*>(obj);
    if (q) {
	    q->Run();
    }
    return NULL;

}

void Thread::Start(){

    pthread_create(&thread,NULL,Thread::BootstrapThread,this);

}

// TODO: This really should be static
void Thread::Kill(){
    Signal(SIGKILL);
}

// TODO: This really should be static
void Thread::Yield(){
	pthread_yield();
}

void Thread::Signal(int signum){
    pthread_kill(thread,signum);
}

Thread::~Thread(){
}

}
