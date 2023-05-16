/*

    libeutils - http://www.excito.com/

    FileUtils.h - this file is part of libeutils.

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

#ifndef MY_URL_H
#define MY_URL_H

#include <string>

using namespace std;

namespace EUtils{

class URL{
private:
	URL(const URL&);
	URL& operator=(const URL&);

	string m_url;
	string m_scheme;
	string m_user;
	string m_password;
	string m_host;
	string m_port;
	string m_path;
	string m_extension;
protected:
	virtual void ParseScheme();
	virtual void ParseUser();
	virtual void ParsePassword();
	virtual void ParseHost();
	virtual void ParsePort();
	virtual void ParsePath();
	virtual void ParseExtension();

public:
	URL(string url="");
	void SetUrl(string url);
	const string& GetUrl() const {return m_url;}
	const string& Scheme() const {return m_scheme;}
	const string& User()const {return m_user;}
	const string& Password()const{return m_password;}
	const string& Host()const{return m_host;}
	const string& Port()const{return m_port;}
	const string& Path()const{return m_path;}
	const string& Extension()const{return m_extension;}

	static bool IsHex(unsigned char first, unsigned char sec);
	static string UrlCode(const string &s) ;
	static string UrlDecode(const string &s);
	virtual ~URL();
};
}
#endif
