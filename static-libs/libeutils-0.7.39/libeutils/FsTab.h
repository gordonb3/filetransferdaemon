/*

    libeutils - http://www.excito.com/

    FsTab.h - this file is part of libeutils.

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
#ifndef FSTAB_H
#define FSTAB_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <map>
#include <string>

using namespace std;

namespace EUtils{

class FsTab{
public:

	typedef struct {
		string device;
		string mount;
		string fstype;
		string opts;
		int freq;
		int passno;
	} Entry;

	FsTab(const string& path="/etc/fstab");
	bool AddEntry(Entry e);
	void Write(ostream& of);
	bool DelByDevice(const string& device);
	bool DelByMount(const string& mount);
	static bool Mount(const string& mount);
	static bool UMount(const string& mount);
	map<string,Entry> GetEntries();
	virtual ~FsTab();
	void Dump();
private:
	map<string,Entry> mounts;
	typedef map<string,Entry>::iterator mountIt;
};

};

#endif
