/*

    libeutils - http://www.excito.com/

    SimpleCfg.cpp - this file is part of libeutils.

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

#include "SimpleCfg.h"

#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdio.h>

#include <libeutils/FileUtils.h>
#include <libeutils/StringTools.h>


using namespace std;

namespace EUtils{

SimpleCfg::SimpleCfg(const string& file):map<string,string>(),ind(0),filename(file){
	list<string> fil=FileUtils::GetContent(filename);
	this->content.resize(fil.size());
	copy(fil.begin(),fil.end(),this->content.begin());

	this->createindex();
}

void SimpleCfg::createindex(){
	this->index.clear();
	this->clear();
	this->ind=0;
	for(vector<string>::iterator sIt=content.begin();sIt!=content.end();sIt++,ind++){

		string line=StringTools::Trimmed(*sIt," ");

		// Skip empty lines and comments
		if( line=="" || line[0]=='#'){
			continue;
		}

		list<string> kval=StringTools::Split(line,'=');
		if(kval.size()!=2){
			// Invalid format
			continue;
		}
		string key=StringTools::Trimmed(kval.front()," ");
		index[key]=ind;
		this->insert(pair<string,string>(key,StringTools::Trimmed(kval.back()," ")));
	}
}

static string addnl(string s){
	return s+"\n";
}


bool SimpleCfg::Writeback(bool create){
	string tfile=filename+".new";

	list<string> cnt;
	cnt.resize(content.size());

	if(create && !Stat::FileExists(filename)){
		FileUtils::Write(filename,"",0640);
	}

	transform(content.begin(),content.end(),cnt.begin(),addnl);

	if(!FileUtils::Write(tfile,cnt,Stat(filename).GetMode())){
		cerr << "Failed to write file"<<endl;
		return false;
	}
	if(rename(tfile.c_str(),filename.c_str())!=0){
		cerr << "Failed to replace file"<<endl;
		return false;
	}
	return true;
}

void SimpleCfg::Update(const string& key,const string& value){
	SimpleCfg::iterator fIt=this->find(key);
	if(fIt!=this->end()){
		(*fIt).second=value;
		content[index[key]]=key+"="+value;
	}else{
		content.push_back(key+"="+value);
		this->insert(pair<string,string>(key,value));
		index[key]=ind;
		ind++;
	}
}

void SimpleCfg::Remove(const string& key){
	SimpleCfg::iterator fIt=this->find(key);
	if(fIt==this->end()){
		return;
	}

	this->content.erase(this->content.begin()+index[key]);
	this->createindex();
}

void SimpleCfg::dump(){
	for(size_t i=0; i<content.size();i++){
		cout <<content[i]<<endl;
	}
}

string SimpleCfg::ValueOrDefault(const string& key, const string& def){
	if(this->find(key)==this->end()){
		return def;
	}
	return map<string,string>::operator[](key);
}

}
