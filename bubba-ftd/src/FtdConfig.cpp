/*
    
    DownloadManager - http://www.excito.com/
    
    FtdConfig.cpp - this file is part of DownloadManager.
    
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
    
    $Id: FtdConfig.cpp 1166 2007-12-06 23:10:22Z tor $
*/

#include "FtdConfig.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

FtdConfig& FtdConfig::Instance(){
	static FtdConfig fcfg(CFGPATH);
	
	return fcfg;
}

string FtdConfig::GetStringOrDefault(const string& group,const string& key,const string& def){
	try{
		return this->GetString(group,key);
	}catch(std::runtime_error& err){
		return def;
	}
}

string FtdConfig::GetValueOrDefault(const string& group,const string& key,const string& def){
	try{
		return this->GetValue(group,key);
	}catch(std::runtime_error& err){
		return def;
	}
}


bool FtdConfig::GetBoolOrDefault(const string& group,const string& key,bool def){
	try{
		return this->GetBool(group,key);
	}catch(std::runtime_error& err){
		return def;
	}
}

int FtdConfig::GetIntegerOrDefault(const string& group,const string& key,int def){
	try{
		return this->GetInteger(group,key);
	}catch(std::runtime_error& err){
		return def;
	}
}

FtdConfig::~FtdConfig()
{
}
