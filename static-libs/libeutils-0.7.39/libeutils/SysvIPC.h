/*

    libeutils - http://www.excito.com/

    SysvIPC.h - this file is part of libeutils.

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

#ifndef SYSVIPC_H_
#define SYSVIPC_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <stdexcept>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>

using namespace std;

namespace EUtils {

class IpcError{
    string errmsg;

public:

	IpcError(string msg=""):errmsg(msg){
    }

    virtual ~IpcError(){
    }

    virtual const string what(){
        return errmsg+": "+string(strerror(errno));
    }
};

class SysvIPC {
protected:
    key_t tok;

public:
	SysvIPC(string path, int token);
	virtual ~SysvIPC() noexcept(false);
};

}

#endif /* SYSVIPC_H_ */
