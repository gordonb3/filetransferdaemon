/*

    libeutils - http://www.excito.com/

    FileUtils.cpp - this file is part of libeutils.

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

#include "FileUtils.h"
#include "UserGroups.h"
#include "StringTools.h"
#include "EExcept.h"

#include <stdexcept>
#include <fstream>

#include <ext/stdio_filebuf.h>

#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

namespace EUtils{

Stat::Stat(string path){
	if(stat(path.c_str(),&this->st)){
		EExcept::HandleErrno(errno);
	}
}

bool Stat::FileExists(const string &path){
	struct stat st;
	if(stat(path.c_str(),&st)){
		try{
			EExcept::HandleErrno(errno);
		}catch(EExcept::ENoent err){
			return false;
		}
	}
	return S_ISREG(st.st_mode);
}

bool Stat::DirExists(const string &path){
	struct stat st;
	if(stat(path.c_str(),&st)){
		try{
			EExcept::HandleErrno(errno);
		}catch(EExcept::ENoent err){
			return false;
		}
	}
	return S_ISDIR(st.st_mode);
}

string Stat::GetOwner(){
	return User::UIDToUser(st.st_uid);
}

string Stat::GetGroup(){
	return Group::GIDToGroup(st.st_gid);
}

string FileUtils::GetContentAsString(const string& path){
	string ret;
	ifstream in(path.c_str(),ifstream::in);
	string line;
	while(in.good()){
		getline(in,line);
		if(line.size()>0 || !in.eof()){
			ret+=line;
		}
	}
	in.close();
	return ret;
}
list<string> FileUtils::GetContent(const string& path){
	list<string> ret;
	ifstream in(path.c_str(),ifstream::in);
	string line;
	while(in.good()){
		getline(in,line);
		if(line.size()>0 || !in.eof()){
			ret.push_back(line);
		}
	}
	in.close();
	return ret;
}

bool FileUtils::Write(const string& name, const string& content, mode_t mode){
	int fd;
	if((fd=open(name.c_str(),O_WRONLY|O_CREAT|O_TRUNC,mode))<0){
		return false;
	}

	{
		__gnu_cxx::stdio_filebuf<char> fb(fd,std::ios_base::out);
		iostream of(&fb);

		of<<content<<flush;
	}

	close(fd);

	return true;
}

bool FileUtils::Write(const string& name, list<string>& content, mode_t mode){
	int fd;
	if((fd=open(name.c_str(),O_WRONLY|O_CREAT|O_TRUNC,mode))<0){
		return false;
	}

	{
		__gnu_cxx::stdio_filebuf<char> fb(fd,std::ios_base::out);
		iostream of(&fb);

		for(list<string>::iterator sIt=content.begin();sIt!=content.end();sIt++){
			of << *sIt;
		}

		of<<flush;
	}

	close(fd);

	return true;
}


list<string> FileUtils::ProcessRead(const string& cmd, bool chomp){
	FILE* fil=popen(cmd.c_str(),"r");
	if(!fil){
			EExcept::HandleErrno(errno);
	}
	char line[1024];
	list<string> ret;
	while(!feof(fil)){
		if(fgets(line,1024,fil)){
			ret.push_back(chomp?StringTools::Chomp(line):line);
		}
	}
	if(pclose(fil)){
			EExcept::HandleErrno(errno);
	}
	return ret;
}

list<string> FileUtils::Glob(const string& p){
	glob_t gb;
	int ret;

	if((ret=glob(p.c_str(),GLOB_NOSORT,NULL,&gb))){
		if(ret!=GLOB_NOMATCH){
			EExcept::HandleErrno(errno);
		}
	}

	list<string> res;
	for(unsigned int i=0;i<gb.gl_pathc;i++){
		res.push_back(gb.gl_pathv[i]);
	}

	globfree(&gb);

	return res;
}

void FileUtils::MkDir(const string &path, mode_t mode){
	if(mkdir(path.c_str(),mode)){
		EExcept::HandleErrno(errno);
	}
}

void FileUtils::MkPath(string path,mode_t mode){
	if(path[path.length()-1]!='/'){
		path+="/";
	}

	string::size_type pos=0;
	while((pos=path.find("/",pos))!=string::npos){
		if(pos!=0){
			try{
				Stat s=Stat(path.substr(0,pos));
			}catch(EExcept::ENoent err){
				// Not excisting
				MkDir(path.substr(0,pos).c_str(),mode);
			}

		}
		pos++;
	}

}

void FileUtils::Chown(const string& path, uid_t uid, gid_t gid){
	if(chown(path.c_str(),uid,gid)){
		EExcept::HandleErrno(errno);
	}
}

void FileUtils::Chown(const string& path, const string& user,const string& group){
	Chown(path,User::UserToUID(user),Group::GroupToGID(group));
}

void FileUtils::Chown(const string& path, uid_t uid,const string& group){
	Chown(path,uid,Group::GroupToGID(group));
}

void FileUtils::Chown(const string& path, const string& user,gid_t gid){
	Chown(path,User::UserToUID(user),gid);
}

void FileUtils::CopyFile(const char* srcfile, const char* dstfile){
  std::ifstream src(srcfile);
  std::ofstream dst(dstfile);
  dst << src.rdbuf();
  src.close();
  if(src.fail()) {
    throw runtime_error("Failed to read from file");
  }
  dst.close();
  if(dst.fail()) {
    throw runtime_error("Failed to write to file");
  }
}

bool FileUtils::IsWritable(const char* dir, const char* user){
	Stat st(dir);

	// Is it writeable at all?
	if(!(st.GetMode()&(S_IWUSR|S_IWGRP|S_IWOTH))){
		return false;
	}

	// Is it writeable by others?
	/*
	if(st.GetMode()&S_IWOTH){
		cout << "Writeable by others"<<endl;
		return true;
	}
	*/

	// Is it owned by user and is writeable by owner?
	if( (st.GetMode()&S_IWUSR) && (User::UIDToUser(st.GetUID())==user)){
		return true;
	}

	// Do we belong to group owning file?
	bool ourgroup=false;
	list<pair<string,gid_t> > gr=User::Groups(user);
	list<pair<string,gid_t> >::iterator grIt=gr.begin();
	while(grIt!=gr.end()){
		if((*grIt).second==st.GetGID()){
			ourgroup=true;
		}
		*grIt++;
	}

	// Is it writeable by group and user is in that group?
	if((st.GetMode()&S_IWGRP) && ourgroup){
		return true;
	}

	// Is it writable by others and we aint owner or group.
	if( (st.GetMode()&S_IWOTH) && !(User::UIDToUser(st.GetUID())==user) && !ourgroup){
		return true;
	}

	return false;
}

}
