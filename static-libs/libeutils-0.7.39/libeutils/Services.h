/*

    libeutils - http://www.excito.com/

    Services.h - this file is part of libeutils.

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

#ifndef SERVICES_H_
#define SERVICES_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>
#include <sys/types.h>

using namespace std;

namespace EUtils{
namespace Services{
	bool IsRunning(const string& service);
	bool IsEnabled(const string& service);

	pid_t GetPid(const string& service);

	bool Start(const string& service);
	bool Stop(const string& service);
	bool Reload(const string& service);

	bool Enable(const string& service, int slevel=0, int klevel=0);
	bool Enable(const string& service, int sprio,const list<int>& slev, int kprio, const list<int>& klev);
	bool Disable(const string& service);

}
}

#endif /* SERVICES_H_ */
