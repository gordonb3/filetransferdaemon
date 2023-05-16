/*

    libeutils - http://www.excito.com/

    NetClient.h - this file is part of NetDaemon.

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


#ifndef NETCLIENT_H_
#define NETCLIENT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libeutils/Socket.h>

namespace EUtils{

class NetClient: public UnixClientStreamSocket{
	string daemon;
	void SpawnDaemon();
public:
	NetClient(const string& daemon, const string& path);

	virtual ~NetClient();
};
}
#endif /* NETCLIENT_H_ */
