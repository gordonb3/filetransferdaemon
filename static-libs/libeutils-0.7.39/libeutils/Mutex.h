/*

    libeutils - http://www.excito.com/

    Mutex.h - this file is part of libeutils.

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

#ifndef MYLOCK_H
#define MYLOCK_H

#include <pthread.h>

class Condition;
namespace EUtils{

#define MUTEX_NORMAL 0
#define MUTEX_RECURSIVE 1
#define MUTEX_ERRORCHECK 2

class Mutex{
private:
    pthread_mutex_t lock;
protected:
    pthread_mutex_t* GetPThreadMutex();
public:
    Mutex(int type=MUTEX_NORMAL);
    void Lock();
    void Unlock();
    virtual ~Mutex();
    friend class Condition;
};
}

#endif
