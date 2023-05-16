/*

    libeutils - http://www.excito.com/

    PHPSession.h - this file is part of libeutils.

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

#ifndef PHPSESSION_H_
#define PHPSESSION_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>

using namespace std;

namespace EUtils{

class PHPSession: public map<string,string>
{
	string ParseValue(string& val);
	string ParseBool(string& val);
	string ParseInt(string& val);
	string ParseString(string& val);
public:
	PHPSession(string path);
	virtual ~PHPSession();
};

}

#endif /*PHPSESSION_H_*/
