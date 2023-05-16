/*

    libeutils - http://www.excito.com/

    AsyncWorker.h - this file is part of libeutils.

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

/*******************************************************************
*
*    DESCRIPTION: Asynchrounous worker class
*
*    AUTHOR: "Tor Krill" <tor@excito.com>
*
*    DATE:7/28/2005
*
*******************************************************************/

#ifndef ASYNCWORKER_H
#define ASYNCWORKER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/signal.h>

#include "Mutex.h"
#include "Condition.h"
#include "Thread.h"

#ifdef __cplusplus
#include <list>
#endif

#ifdef __cplusplus

using namespace std;

namespace EUtils{

/**
 * AsyncWorker is a class that perfoms work in a separate thread.
 * Application adds work to a queue which the worker thread consumes
 * entries and processes those.
 *
 * Payload is a simple void* which has to be allocated from heap since
 * Asyncworker will delete it after processing.
 *
 * The correct way to use this is to subclass it and override the ProcessElem
 * method.
 */
class AsyncWorker:public Thread{
private:
    /**
     * Lock guarding the inner queue in the form of a list.
     */
    Mutex listlock;

    /**
     * Should thread keep running?
     */
    bool running;

    /**
     * Conditional used to signal thread that data has been added to queue
     */
    Condition data_available;

    /**
     * Queue holding workload
     */
    list<void*> write_buffer;

    /**
     * sigacition for installment of signal handler.
     */
    struct sigaction actions;

    virtual void Run();

    /**
     * Mainloop consuming queue
     */
    void ProcessQueue();
    /**
     * Helper method which retrieves one element from queue in a threadsafe
     * manner.
     *
     * @return One buffer or NULL if queue is empty.
     */
    void* Get();
    /**
     * Notify worker thread of new data in queue
     */
    void NotifyWorker();

public:
    /**
     * Default constructor. Creates an instance.
     */
    AsyncWorker();
    /**
     * Destructor, releases aquired resources and terminate worker thread.
     */
    virtual ~AsyncWorker();

    /**
     * Add a new element last in queue.
     *
     * @param buf    Element to add
     */
    void AddLast(void* buf);
    /**
     * Add new element first in queue
     *
     * @param buf    Element to add
     */
    void AddFirst(void* buf);
    /**
     * Empty queue and release memory.
     */
    void Flush();
    /**
     * Send interruption signal to worker thread. If in interuptible sleep
     * thread will drop current element and continue with next one.
     */
    void Cancel();
    /**
     * Retrieve size of queue.
     *
     * @return size of queue.
     */
    int Size();

    /**
     * Virtual function called for each element to process. Override this to
     * do other processing.
     *
     * @param b      Element to process.
     */
    virtual void ProcessElem(void* b);

};

}

#endif /* _cplusplus */

#endif /* ASYNCWORKER_H */
