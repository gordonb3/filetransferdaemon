/*

    libeutils - http://www.excito.com/

    UserGroups.h - this file is part of libeutils.

    Copyright (C) 2008 Tor Krill <tor@excito.com>

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
#ifndef USERGROUPS_H_
#define USERGROUPS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>
#include <grp.h>

using namespace std;

namespace EUtils{

class User{
public:
	static uid_t UserToUID(string username);
	static string UIDToUser(uid_t uid);
	static list<pair<string,gid_t> > Groups(string user);
};

class Group{
public:
	static gid_t GroupToGID(string groupname);
	static string GIDToGroup(gid_t gid);

};
}
#endif /*USERGROUPS_H_*/
