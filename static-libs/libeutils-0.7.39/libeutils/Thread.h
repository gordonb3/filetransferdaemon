/*

    libeutils - http://www.excito.com/

    Thread.h - this file is part of libeutils.

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

#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <pthread.h>
#include "Condition.h"

namespace EUtils{

class Thread{
private:
     /**
     * Threadid for worker thread.
     */
    pthread_t thread;

    static void* BootstrapThread(void* inst);

protected:

public:
    Thread();

    virtual void Start();

    virtual void Run(){};

    virtual void Signal(int signum);

    virtual void Kill();

    virtual void Yield();

    virtual ~Thread();

    friend class Condition;

};

}
#endif
