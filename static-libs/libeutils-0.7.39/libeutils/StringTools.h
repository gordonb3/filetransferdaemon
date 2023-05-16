/*

    libeutils - http://www.excito.com/

    StringTools.h - this file is part of libeutils.

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

#ifndef STRINGTOOLS_H_
#define STRINGTOOLS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

using namespace std;

namespace EUtils{

class StringTools
{
public:
	static list<string> Split(string const& str, const char delim);
	static list<string> Split(string const& str, const string& regex);
	static string Chomp(const string& str);
	static string ToLower(string str);
	static string ToUpper(string str);
	static string Trimmed( string const& str, char const* sepSet);
	static string GetPath(const string& s);
	static string GetFileName(const string& s);
	static string UrlDecode(const string& s);
	static string SizeToHuman(unsigned long long val);
	static string SimpleUUID(void);
	static string convertMac(long long mac);
	static long long convertMac(const string& mac);
};
}
#endif /*STRINGTOOLS_H_*/
