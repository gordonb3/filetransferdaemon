/*

    libeutils - http://www.excito.com/

    PHPSession.cpp - this file is part of libeutils.

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

#include "PHPSession.h"
#include "StringTools.h"

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <stdlib.h>

namespace EUtils{

PHPSession::PHPSession(string path)
{
	ifstream in(path.c_str());
	if(in.is_open()){
		while(!in.eof()){
			string line;
			getline(in,line);
			if(line.length()){
				list<string> vars=StringTools::Split(line,';');
				for(list<string>::iterator lIt=vars.begin();lIt!=vars.end();lIt++){
					if((*lIt).length()){
						list<string> var=StringTools::Split(*lIt,'|');
						if(var.size()==2){
							this->operator[](var.front())=this->ParseValue(var.back());
						}
					}
				}
			}
		}
	}

}

string PHPSession::ParseValue(string& val){
	char type=val[0];
	string res;
	switch(type){
	case 'b':
		res=this->ParseBool(val);
		break;
	case 's':
		res=this->ParseString(val);
		break;
	case 'i':
		res=this->ParseInt(val);
		break;
	default:
		// Unknown type
		// TODO: Implement others, fx a-array
		break;
	}

	return res;
}

string PHPSession::ParseBool(string& val){

	return val.substr(2);
}

string PHPSession::ParseString(string& val){
	string res;
	string::size_type index=val.find(":",2);
	if(index!=string::npos){
		int len=atoi(val.substr(2,index-2).c_str());
		string::size_type pos=val.find("\"");
		if(pos!=string::npos){
			res=val.substr(pos+1,len);
		}
	}
	return res;
}

string PHPSession::ParseInt(string& val){

	list<string> kval=StringTools::Split(val,':');
	if(kval.size()==2){
		return kval.back();
	}else{
		return "N/A";
	}

}

PHPSession::~PHPSession()
{
}

}
