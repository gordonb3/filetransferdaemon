/*

    libeutils - http://www.excito.com/

    FsTab.cpp - this file is part of libeutils.

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

#include "FsTab.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <stdlib.h>

#include "StringTools.h"
#include "Process.h"

//TODO: refactor this into eUtils :(
static int do_call(const string& cmd){
	int ret=system(cmd.c_str());
	if(ret<0){
		return ret;
	}
	return WEXITSTATUS(ret);
}


namespace EUtils{


FsTab::FsTab(const string& path){

	ifstream in(path.c_str(),ifstream::in);
	string line;
	while(in.good()){
		getline(in,line);
		line=StringTools::Trimmed(line," \t");
		if(line[0]=='#'){
			continue;
		}
		if(line==""){
			continue;
		}
		list<string> el=StringTools::Split(line,"[[:blank:]]+");

		if(el.size()==6){
			list<string>::iterator sIt=el.begin();
			Entry entry;
			for(int i=0;sIt!=el.end();sIt++){
				switch(i){
				case 0:
					entry.device=*sIt;
					break;
				case 1:
					entry.mount=*sIt;
					break;
				case 2:
					entry.fstype=*sIt;
					break;
				case 3:
					entry.opts=*sIt;
					break;
				case 4:
					entry.freq=atoi((*sIt).c_str());
					break;
				case 5:
					entry.passno=atoi((*sIt).c_str());
					break;
				}
				i++;
			}
			this->mounts[entry.device]=entry;
		}
	}
	in.close();
}

map<string,FsTab::Entry> FsTab::GetEntries(){
	return this->mounts;
}

bool FsTab::AddEntry(Entry e){
	if(this->mounts.find(e.device)==this->mounts.end()){
		this->mounts[e.device]=e;
		return true;
	}else{
		return false;
	}
}

bool FsTab::DelByDevice(const string& device){
	mountIt fIt;
	if((fIt=this->mounts.find(device))!=this->mounts.end()){
		this->mounts.erase(fIt);
		return true;
	}else{
		return false;
	}
}

bool FsTab::DelByMount(const string& mount){
	mountIt mIt=this->mounts.begin();
	mountIt sIt=this->mounts.end();
	for(;mIt!=mounts.end();mIt++){
		if((*mIt).second.mount==mount){
			sIt=mIt;
			break;
		}
	}
	if(sIt!=this->mounts.end()){
		this->mounts.erase(sIt);
		return true;
	}else{
		return false;
	}
}


void FsTab::Write(ostream& of){
	mountIt mIt=this->mounts.begin();
	for(;mIt!=mounts.end();mIt++){
		of << (*mIt).second.device << "\t"
			<< (*mIt).second.mount << "\t"
			<< (*mIt).second.fstype << "\t"
			<< (*mIt).second.opts << "\t"
			<< (*mIt).second.freq << "\t"
			<< (*mIt).second.passno << endl;
	}
}

bool FsTab::Mount(const string& mount){
	const char* cmd[] = {
		"/bin/mount",
		mount.c_str(),
		NULL
	};

	Process p;
	if( p.call( cmd ) != 0 ) {
		return false;
	}

	return true;
}

bool FsTab::UMount(const string& mount){

	{
		const char* cmd[] = {
			"/bin/fuser",
			"-kms",
			mount.c_str(),
			NULL
		};

		Process p;
		p.call( cmd );
	}

	{
		const char* cmd[] = {
			"/bin/umount",
			mount.c_str(),
			NULL
		};

		Process p;
		if( p.call( cmd ) != 0 ) {
			return false;
		}
	}

	return true;
}


FsTab::~FsTab(){
}

void FsTab::Dump(){
	cout << "fstab contains "<< this->mounts.size()<<" entries."<<endl;
	mountIt mIt=this->mounts.begin();
	for(;mIt!=mounts.end();mIt++){
		cout << "Device: "<<(*mIt).second.device<<endl;
		cout << "================================="<<endl;
		cout << "Mount : "<<(*mIt).second.mount<<endl;
		cout << "Fstype: "<<(*mIt).second.fstype<<endl;
		cout << "Opts  : "<<(*mIt).second.opts<<endl;
		cout << "Freq  : "<<(*mIt).second.freq<<endl;
		cout << "Passno: "<<(*mIt).second.passno<<endl;
		cout << endl;
	}
}
}
