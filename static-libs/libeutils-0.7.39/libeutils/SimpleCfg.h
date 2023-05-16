/*

    libeutils - http://www.excito.com/

    SimpleCfg.h - this file is part of libeutils.

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
#ifndef SIMPLECFG_H_
#define SIMPLECFG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <map>
#include <string>
#include <vector>

using namespace std;

namespace EUtils{

class SimpleCfg: public map<string,string>{
public:

	SimpleCfg(const string& file);

	bool Writeback(bool create=true);

	void Update(const string& key,const string& value);
	void Remove(const string& key);

	void dump();

	string ValueOrDefault(const string& key, const string& def);

	virtual ~SimpleCfg(){}
private:
	void createindex();
	signed int ind;
	string filename;
	vector<string> content;
	map<string,signed int> index;
};

}

#endif /* SIMPLECFG_H_ */
