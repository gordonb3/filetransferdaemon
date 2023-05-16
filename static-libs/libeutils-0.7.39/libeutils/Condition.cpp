/*

    libeutils - http://www.excito.com/

    Condition.cpp - this file is part of libeutils.

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


#include "Condition.h"
#include <stdio.h>

namespace EUtils{

Condition::Condition(){
    pthread_cond_init(&condition,NULL);
}

void Condition::Wait(){
    lock.Lock();

    if (pthread_cond_wait(&condition, lock.GetPThreadMutex())) {
        // Throw exception here
        perror("Failed to wait");
    }

    lock.Unlock();
}

void Condition::Notify(){
    lock.Lock();
    if (pthread_cond_signal(&condition)) {
        perror("Failed to signal writer thread");
    }
    lock.Unlock();
}

void Condition::NotifyAll(){
    lock.Lock();
    if (pthread_cond_broadcast(&condition)) {
        perror("Failed to signal writer thread");
    }
    lock.Unlock();
}

Condition::~Condition(){
}

}
