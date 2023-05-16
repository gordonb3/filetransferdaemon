/*

    libeutils - http://www.excito.com/

    Regex.h - this file is part of libeutils.

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
#ifndef REGEX_H
#define REGEX_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <regex.h>
#include <string>
#include <vector>

using namespace std;

namespace EUtils{

class Regex{
private:
	// Do we have a valid compiled regex
	bool regvalid;
	// The regex we currently work on
	regex_t preg;

	// Result set.
	// Max results per match.
	int maxmatch;
	// Do we process a result right now
	bool active;
	// String to process
	string curline;
	// Current position in string
	regoff_t curpos;

	void FreeRegex();
	string GetError(int err);
public:
	typedef pair<regoff_t,regoff_t> RegMatch;
	typedef vector<RegMatch> Matches;
	Regex():regvalid(false){};
	Regex(const string& regex, bool caseinsens=false);

	bool Match(const string& val, int maxmatch=10);

	unsigned int NextMatch(Matches& match);

	void Compile(const string& regex, bool caseinsens=false);

	virtual ~Regex();
};

};
#endif

