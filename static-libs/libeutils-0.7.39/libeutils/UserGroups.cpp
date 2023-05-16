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
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <stdlib.h>

#include "Mutex.h"
#include "UserGroups.h"
#include "EExcept.h"

namespace EUtils{

static Mutex umutex;
static Mutex gmutex;


gid_t Group::GroupToGID(string gn){
	struct group* grp;
	gmutex.Lock();
	if(!(grp=getgrnam(gn.c_str()))){
		gmutex.Unlock();
		EExcept::HandleErrno(errno);
	}
	gmutex.Unlock();
	return grp->gr_gid;
}

string Group::GIDToGroup(gid_t gid){
	struct group* grp;
	gmutex.Lock();
	if(!(grp=getgrgid(gid))){
		gmutex.Unlock();
		EExcept::HandleErrno(errno);
	}
	gmutex.Unlock();
	return grp->gr_name;
}

list<pair<string,gid_t> > User::Groups(string user){
	list<pair<string,gid_t> > res;
	int ngroups=20;
	gid_t *groups;
	struct passwd *pw;
	struct group *gr;

	umutex.Lock();

	groups =(gid_t *) malloc(ngroups * sizeof (gid_t));
	if (groups == NULL) {
		umutex.Unlock();
		return res;
	}
	pw = getpwnam(user.c_str());
	if (pw == NULL) {
		umutex.Unlock();
		free(groups);
		return res;
	}

	if (getgrouplist(user.c_str(), pw->pw_gid, groups, &ngroups) == -1){
		umutex.Unlock();
		free(groups);
		return res;
	}

	for (int j = 0; j < ngroups; j++) {
		gr = getgrgid(groups[j]);
		if (gr != NULL){
			res.push_back(pair<string,gid_t>(gr->gr_name,gr->gr_gid));
		}
	}
	free(groups);
	umutex.Unlock();
	return res;
}

uid_t User::UserToUID(string un){
	struct passwd* pwd;
	umutex.Lock();
	if(!(pwd=getpwnam(un.c_str()))){
		umutex.Unlock();
		EExcept::HandleErrno(errno);
	}
	umutex.Unlock();
	return pwd->pw_uid;
}

string User::UIDToUser(uid_t uid){
	struct passwd* pwd;
	umutex.Lock();
	if(!(pwd=getpwuid(uid))){
		umutex.Unlock();
		EExcept::HandleErrno(errno);
	}
	umutex.Unlock();
	return pwd->pw_name;
}

}
