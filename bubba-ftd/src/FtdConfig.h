/*
    
    DownloadManager - http://www.excito.com/
    
    FtdConfig.h - this file is part of DownloadManager.
    
    Copyright (C) 2007 Tor Krill <tor@excito.com>
    
    DownloadManager is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    DownloadManager is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with DownloadManager; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id: FtdConfig.h 2144 2008-11-17 21:02:28Z tor $
*/

#ifndef FTDCONFIG_H_
#define FTDCONFIG_H_

#include <libeutils/EConfig.h>

using namespace EUtils;

class FtdConfig: public EConfig
{
public:
	FtdConfig(string cfgpath):EConfig(cfgpath){}
	
	/*
	 * Convenience wrappers. If value not set in config return provided default value.
	 * Catches any exceptions or failures.
	 */
	string GetValueOrDefault(const string& group,const string& key,const string& def);
	string GetStringOrDefault(const string& group,const string& key,const string& def);
	bool GetBoolOrDefault(const string& group,const string& key, bool def);
	int GetIntegerOrDefault(const string& group,const string& key,int def);
	
	virtual ~FtdConfig();
	static FtdConfig& Instance(void);
};

#endif /*FTDCONFIG_H_*/
