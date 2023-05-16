/*

    libeutils - http://www.excito.com/

    POpt.cpp - this file is part of libeutils.

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

    $Id:$
*/

#ifndef EX_POPT_H
#define EX_POPT_H

#include <popt.h>

#include <map>
#include <list>
#include <string>

using namespace std;

namespace EUtils{

class Option{
public:
	enum Type{
		None=POPT_ARG_NONE,
		String=POPT_ARG_STRING,
		Int=POPT_ARG_INT,
		Long=POPT_ARG_LONG,
#ifdef POPT_ARG_LONGLONG
		Longlong=POPT_ARG_LONGLONG,
#endif
		Float=POPT_ARG_FLOAT,
		Double=POPT_ARG_DOUBLE
	};
	enum Type type;
	string longName;
	char shortName;
	string desc;
	string argDesc;
	string value;

	Option():shortName('\0'){}
	Option(string longName, char shortName,enum Type type, string description, string adesc,string defval=""):
		type(type),longName(longName),shortName(shortName),desc(description),argDesc(adesc),value(defval){};
};

class POpt{
private:
	static void parsecallback(poptContext con, enum poptCallbackReason reason,
								const struct poptOption * opt, const char * arg,
								void * data);
	map<string,Option> options;

	struct poptOption* parse_options;
	void buildparseopts();
	void cleanparseopts();
	list<string> remainder;

public:
	POpt();
	POpt(list<Option> opts);

	void AddOption(const Option& opt);

	bool Parse(int argc, char** argv);

	string& GetValue(const char* longname);
	string& operator[](const char* longname);
	Option& GetOption(const char* longname);
	list<string> Remainder();

	virtual ~POpt();
};

}
#endif
