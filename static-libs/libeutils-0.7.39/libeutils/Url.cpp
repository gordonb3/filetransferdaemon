/*

    libeutils - http://www.excito.com/

    URL.cpp - this file is part of libeutils.

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

#include <cctype>
#include <algorithm>
#include <stdio.h>

#include "Url.h"

namespace EUtils{

URL::URL(string url){
	if (!url.empty()) {
		this->SetUrl(url);
	}
}

void URL::SetUrl(string url){
	// Needed for parse methods to work
	m_url=url;

	this->ParseScheme();
	// Hack to support Magnet links in "standard" way
	if(this->Scheme()=="magnet"){
		// Dont touch
		this->m_url=url;
	}else{
		this->ParseUser();
		this->ParsePassword();
		this->ParseHost();
		this->ParsePort();
		this->ParsePath();
		this->ParseExtension();

		this->m_url=this->Scheme()+"://";

		if (!m_user.empty()) {
			m_url+=m_user;
			if (!m_password.empty()) {
				m_url+=":"+m_password;
			}
			m_url+="@";
		}

		m_url+=m_host;

		if (!m_port.empty()) {
			m_url+=":"+m_port;
		}
		if (!m_path.empty()) {
			m_url+="/"+m_path;
		}
	}
}

void URL::ParseScheme(){
	int index=m_url.find(":",0);
	if(index<0){
		return;
	}
	m_scheme=m_url.substr(0,index);

	int (*pf)(int)=tolower;
	transform(m_scheme.begin(), m_scheme.end(), m_scheme.begin(), pf);
}

void URL::ParseUser(){
	// Find initial //
	int start=m_url.find("//",0);
	if(start<0){
			return;
	}

	// Does url contain user info?
	int end=m_url.find("@");

	if(end<0){
		// No
		return;
	}
	string t=m_url.substr(start+2,end-(start+2));
	if((start=t.find(":"))>0){
		// Got a separator user comes before :
		t=t.substr(0,start);
	}else{
		// Only got a password, strange people
		return;
	}
	m_user=t;

}

void URL::ParsePassword(){
	// Find initial //
	int start=m_url.find("//",0);
	if(start<0){
			return;
	}

	// Does url contain user info?
	int end=m_url.find("@");

	if(end<0){
		// No
		return;
	}
	string t=m_url.substr(start+2,end-(start+2));
	if((start=t.find(":"))>=0){
		// Got a separator password comes after :
		t=t.substr(start+1);
	}else{
		return;
	}
	if(t.length()>0){
		m_password=t;
	}
}

void URL::ParseHost(){

	// Find initial //
	int start=m_url.find("//",0);
	if(start<0){
			return;
	}
	// Try to find / after host
	int end=m_url.find("/",start+2);
	string t;
	if(end>0){
		t=m_url.substr(start+2,end-(start+2));
	}else{
		t=m_url.substr(start+2);
	}

	// Does url contain user info?
	if((start=t.find("@"))>0){
			// Yes remove it
			t=t.substr(start+1);
	}

	// Does it contain a port?
	if((start=t.find(":"))>0){
		// Yes, remove it
		t=t.substr(0,t.length()-(t.length()-start));
	}
	m_host=t;
	int (*pf)(int)=tolower;
	transform(m_host.begin(), m_host.end(), m_host.begin(), pf);

}

void URL::ParsePort(){
	// Find initial //
	int start=m_url.find("//",0);
	if(start<0){
			return;
	}
	// Try to find / after host
	int end=m_url.find("/",start+2);
	string t;
	if(end>0){
		t=m_url.substr(start+2,end-(start+2));
	}else{
		t=m_url.substr(start+2);
	}

	// Does url contain user info?
	if((start=t.find("@"))>0){
			// Yes remove it
			t=t.substr(start+1);
	}

	// Does it contain a port?
	if((start=t.find(":"))>0){
		// Yes, remove it
		t=t.substr(start+1);
	}else{
		return;
	}
	m_port=t;

}

void URL::ParsePath(){
	// Find initial //
	int start=m_url.find("//",0);
	if(start<0){
			return;
	}

	// Try to find / after host
	int end=m_url.find("/",start+2);
	string t;
	if(end>0){
		t=m_url.substr(end+1);
	}else{
		return;
	}

	m_path=URL::UrlCode(t);

}

// This method is somewhat of a hack since it depends on parsepath have being run
void URL::ParseExtension(){
	if (!m_path.empty()) {
		string path=m_path;
		int (*pf)(int)=tolower;
		transform(path.begin(), path.end(), path.begin(), pf);
		int end=path.rfind(".");
		if (end>=0) {
			m_extension=path.substr(end+1);
		}
	}
}

bool URL::IsHex(unsigned char first, unsigned char sec) {
	if ( isdigit(first)||((tolower(first)>96)&&(tolower(first)<103)) ) {
		if ( isdigit(sec)||((tolower(sec)>96)&&(tolower(sec)<103)) ) {
			return true;
		}
	}
	return false;
}

string URL::UrlCode(const string &s) {
	const char* str=s.c_str();
	unsigned char ch;
	unsigned int i=0;
	string ret;

	while ( (ch=str[i++]) ) {
		if ( isalpha(ch)||isdigit(ch) ) {
			ret.push_back(ch);
			continue;
		}
		switch ( ch ) {
		case '/':
		case ':':
		case '_':
		case '.':
		case '-':
		case '?':
		case '=':
		case '&':
			ret.push_back(ch);
			continue;
			break;
		case '%':
			if ( (i+1<=s.length()) && (URL::IsHex(str[i],str[i+1])) ) {
				ret.push_back(ch);
				continue;
			}
			break;
		default:
			break;
		}
		char buf[8];
		sprintf(buf,"%%%02x",ch);
		ret.append(buf);
	}

	return ret;
}

string URL::UrlDecode(const string &s){
	const char* str=s.c_str();
	unsigned char ch;
	unsigned int i=0;
	string ret;

	while ( (ch=str[i++]) ) {
		if(ch=='+'){
			ret.push_back(' ');
			continue;
		}

		if(ch=='%' && (i+1<=s.length()) && (URL::IsHex(str[i],str[i+1]))){
			ret.push_back(str[i]*16+str[i+1]);
			i+=2;
			continue;
		}

		ret.push_back(ch);
	}
	return ret;
}

URL::~URL(){
}

}
