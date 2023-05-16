/*

    libeutils - http://www.excito.com/

    EExcept.h - this file is part of libeutils.

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

#ifndef EEXCEPT_H_
#define EEXCEPT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <string>

namespace EUtils{

namespace EExcept{

class EAccess: public std::runtime_error{
public:
	EAccess(std::string arg):std::runtime_error(arg){};
};

class ENoent: public std::runtime_error{
public:
	ENoent(std::string arg):std::runtime_error(arg){};
};

void HandleErrno(int err);

}

}

#endif /*EEXCEPT_H_*/
