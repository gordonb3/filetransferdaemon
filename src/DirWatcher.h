/*
    
    DownloadManager - http://www.excito.com/
    
    DirWatcher.h - this file is part of DownloadManager.
    
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
    
    $Id: DirWatcher.h 2133 2008-11-12 22:24:50Z tor $
*/

#ifndef DIRWATCHER_H_
#define DIRWATCHER_H_

#include <map>

#include <libeutils/Thread.h>
#include "FtdApp.h"
#include "TorrentDownloader.h"

class DirWatcher: public Thread{
	int inotifyfd;
	bool doRun;
	map<int,string> iwatches;
	string baseglob;
	TorrentDownloadManager* mngr;
	FtdApp* app;
	
	DirWatcher(const DirWatcher&);
	DirWatcher& operator=(const DirWatcher&);	
	
protected:
	void LoadInitial();
	void AddWatches();
	void AddHomeWatch();
	void AddDownload(const string& path);
	void DownloadFailed(const string& err,void* dl);
public:
	static DirWatcher& Instance();
	DirWatcher();
	void AddWatch(const string& path);
	void DelWatch(const string& path);
	void Stop();
	virtual void Run();
	virtual ~DirWatcher();
};

#endif /*DIRWATCHER_H_*/
