/*

    libeutils - http://www.excito.com/

    EExcept.cpp - this file is part of libeutils.

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

#include "EExcept.h"
#include <string.h>
#include <errno.h>

namespace EUtils{

namespace EExcept{

void HandleErrno(int err){
		switch(err){
			case EACCES:
				throw EExcept::EAccess(strerror(err));
			break;
			case ENOENT:
				throw EExcept::ENoent(strerror(err));
			break;
			default:
				throw std::runtime_error(strerror(err));
		}
}

}

}
