/*

    libeutils - http://www.excito.com/

    NetDaemon.h - this file is part of NetDaemon.

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

#ifndef MY_NETDAEMON_H
#define MY_NETDAEMON_H

#include <libeutils/Socket.h>
#include <libeutils/Mutex.h>
#include <syslog.h>

#include <string>

using namespace std;

namespace EUtils{

class NetDaemon{
private:
	UnixServerSocket serv;
	int timeout;
	bool shutdown;

	Mutex reqmutex;
	int pendingreq;

	/**
	 * Increment pending requests
	 */
	void increq();
protected:
	/**
	 * Decrement pending requests
	 */
	void decreq();
public:
	NetDaemon(const string& sockpath,int idletimeout);

	/**
	 * Process incomming connection.
	 *
	 * It is the responsibility of the sub class to
	 * call decreq() when request is processed.
	 */
	virtual void Dispatch(UnixClientSocket* con);

	void Run();

	void ShutDown();

	virtual ~NetDaemon();
};

}

#endif
