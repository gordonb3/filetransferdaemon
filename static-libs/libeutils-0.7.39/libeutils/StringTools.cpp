/*

    libeutils - http://www.excito.com/

    StringTools.cpp - this file is part of libeutils.

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

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <sys/time.h>
#include <stdio.h>
#include "StringTools.h"
#include "Regex.h"

namespace EUtils{

string StringTools::Trimmed( string const& str, char const* sepSet){
        string::size_type const first = str.find_first_not_of(sepSet);
        return ( first==string::npos )? string(): str.substr(first, str.find_last_not_of(sepSet)-first+1);
}

string StringTools::ToLower(string str){
        int (*pf)(int)=tolower;
        transform(str.begin(), str.end(), str.begin(), pf);
        return str;
}

string StringTools::ToUpper(string str){
        int (*pf)(int)=toupper;
        transform(str.begin(), str.end(), str.begin(), pf);
        return str;
}

string StringTools::Chomp(const string& str){
	string::size_type pos = str.find_last_not_of("\n");
	return pos==string::npos?str:str.substr(0, pos+1);
}

list<string> StringTools::Split(string const& str, const char delim) {
        list<string> items;
        int pos=-1,oldpos=0;
        do {
                pos=str.find(delim,oldpos);
                if ( oldpos>=0 ) {
                        items.push_back(StringTools::Trimmed(str.substr(oldpos,pos-oldpos)," "));
                }
                oldpos=pos+1;
        }while ( oldpos>0 );

        return items;
}

list<string> StringTools::Split(string const& str, const string& delim) {
        list<string> items;
        EUtils::Regex r("("+delim+")");

        unsigned int pos=-1,oldpos=0;

		if(!r.Match(str)){
			if(str.size()>0){
				items.push_back(str);
			}
			return items;
		}

       	EUtils::Regex::Matches m;
		while(r.NextMatch(m)){
			// Initial match, skip
			if(m[0].first==0){
				oldpos=m[0].second;
				continue;
			}
			//pos==1 char after match
			pos=m[0].second;
           	string entry=str.substr(oldpos,m[0].first-oldpos);
			items.push_back(entry);
			oldpos=pos;
		}

		// if no trailing delimiter
		if(oldpos!=str.size()){
			items.push_back(str.substr(oldpos,str.size()-oldpos));
		}

        return items;
}

string StringTools::GetPath(const string& s){
	string::size_type pos;
	if((pos=s.find_last_of("/"))!=string::npos){
		return s.substr(0,pos);
	}else{
		return s;
	}
}

string StringTools::GetFileName(const string& s){
	string::size_type pos;
	if((pos=s.find_last_of("/"))!=string::npos){
			return s.substr(pos+1);
	}else{
			return s;
	}

}

string StringTools::UrlDecode(const string& s){
	const string vals="0123456789abcdef";
	string res;
	long size=s.size();
	int v1,v2;

	for(int i=0;i<size;i++){
		if((s[i]=='%') && ((i+2)<size) && ((v1=vals.find(tolower(s[i+1])))>=0) && ((v2=vals.find(tolower(s[i+2])))>=0)){
			res+= v2+(16*v1);
			i+=2;
		}else if(s[i]=='+'){
			res+=' ';
		}else{
			res+=s[i];
		}
	}

	return res;

}

string StringTools::SizeToHuman(unsigned long long val){
	stringstream sstr;
	if(val>=1024 && val<1048576){
		sstr<<(val>>10)<<"."<<(val-(val>>10)*(1<<10))/((1<<10)/10+1)<<" K";
	}else if(val>=1048576 && val<1073741824){
		sstr<<(val>>20)<<"."<<(val-(val>>20)*(1<<20))/((1<<20)/10+1)<<" M";
	}else if(val>=1073741824){
		sstr<<(val>>30)<<"."<<(val-(val>>30)*(1<<30))/((1<<30)/10+1)<<" G";
	}else{
		sstr<<val;
	}
	return sstr.str();
}

long long StringTools::convertMac(const string& mac){
	long long v[6];
	sscanf(mac.c_str(),"%llx:%llx:%llx:%llx:%llx:%llx",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
	return (v[0]<<40) | (v[1]<<32) | (v[2]<<24) | (v[3]<<16) | (v[4]<<8) | (v[5]);
}

string StringTools::convertMac(long long mac){
	char buf[20];
	unsigned char m[6];
	m[0]=mac     & 0xff;
	m[1]=mac>>8  & 0xff;
	m[2]=mac>>16 & 0xff;
	m[3]=mac>>24 & 0xff;
	m[4]=mac>>32 & 0xff;
	m[5]=mac>>40 & 0xff;
	sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",m[5],m[4],m[3],m[2],m[1],m[0]);

	return string(buf);
}


string StringTools::SimpleUUID(void){
	stringstream res;
	struct timeval t;
	if(gettimeofday(&t,NULL)){
		// TODO: Errhandling
	}
	res<<"ftd";
	res<<setfill('0')<<setw(8)<<hex<<t.tv_sec;
	res<<setfill('0')<<setw(5)<<hex<<t.tv_usec;
	return res.str();
}

}
