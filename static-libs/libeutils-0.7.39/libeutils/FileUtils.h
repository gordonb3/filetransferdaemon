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
#ifndef MY_FILEUTILS
#define MY_FILEUTILS

#include <list>
#include <string>
#include <sys/stat.h>

using namespace std;

namespace EUtils{

class Stat{
	struct stat st;
public:
	Stat(string path);

	static bool FileExists(const string& path);
	static bool DirExists(const string& path);

	uid_t GetUID(){return st.st_uid;}
	string GetOwner();
	gid_t GetGID(){return st.st_gid;}
	string GetGroup();
	off_t GetSize(){return st.st_size;}
	dev_t GetDevice(){return st.st_dev;}
	mode_t GetMode(){return st.st_mode;};
	time_t GetAccessTime(){return st.st_atime;}
	time_t GetModificationTime(){return st.st_mtime;}
	time_t GetChangeTime(){return st.st_ctime;}

};

class FileUtils{
public:
	static list<string> Glob(const string &pattern);
	static list<string> GetContent(const string& path);
	static string GetContentAsString(const string& path);
	static list<string> ProcessRead(const string& cmd,bool chomp=false);
	static bool Write(const string& name, const string& content, mode_t mode=0660);
	static bool Write(const string& name, list<string>& content, mode_t mode=0660);
	static Stat StatFile(const string &path){return Stat(path);}
	static void MkDir(const string &path,mode_t mode=0770);
	static void MkPath(string path,mode_t mode=0770);
	static void Chown(const string& path, const string& user,const string& group);
	static void Chown(const string& path, uid_t uid,const string& group);
	static void Chown(const string& path, const string& user,gid_t gid);
	static void Chown(const string& path, uid_t uid, gid_t gid);

	static void CopyFile(const char* srcfile, const char* dstfile);
	static bool IsWritable(const char* dir, const char* user);
};
}
#endif
