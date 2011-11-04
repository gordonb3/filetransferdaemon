/*
    
    DownloadManager - http://www.excito.com/
    
    TorrentDownloader.h - this file is part of DownloadManager.
    
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
    
    $Id: TorrentDownloader.h 2015 2008-10-14 13:45:35Z tor $
*/

#ifndef MY_TORRENTDOWNLOADER_H
#define MY_TORRENTDOWNLOADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <list>

#include <libeutils/Mutex.h>
#include <libeutils/Condition.h>
#include "Downloader.h"
#include "CurlDownloader.h"

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"

using namespace libtorrent;

class TorrentDownloadManager;

class TorrentDownloader: public Downloader{
	torrent_handle handle;
	TorrentDownloadManager* mgr;
	CurlDownloader* downloader;
	stringstream m_dlstream;
	string torrentfilename;
	string resumefilename;
	string errmsg;
	bool deletetorrent;
	bool perm_changed;
protected:

	EUtils::Condition m_resumewritten;

	void DeleteTorrent(void);

	void SetDeletestatus(bool dodelete);

	void WriteTorrent(void);
	void DoWriteResumeData(boost::shared_ptr<entry> resume_data);
	void WriteResumeData(void);
	
	void HttpDone();
	void HttpFailed(const string &msg);
	
	void Update();
	
	void DownloadDone();
	
	void ChangePermissions();

	bool AddDownload(std::iostream* str);
	
	static void DeferredCleanup(void*pl);
	
	void DoCancel();
	static void DeferredCancel(void *pl);
	
	void SetTorrentFilename(const string& name){this->torrentfilename=name;}

	TorrentDownloader(TorrentDownloadManager* mgr);

public:

	virtual void StartDownload(void);

	bool StartFromFile(const string& path);

	virtual void CancelDownload();
	virtual void StopDownload();

	virtual Json::Value GetInfo();
	virtual void SetInfo(string info);

	virtual ~TorrentDownloader();

	friend class TorrentDownloadManager;
	friend class DirWatcher;
};

class TorrentDownloadManager:public DownloadManager{
private:
	boost::thread    m_Thread;

	bool isRunning;
	session s;
	
	Mutex dlmutex;
	map<torrent_handle,TorrentDownloader*> handle_map;
	list<TorrentDownloader*> downloaders;
	
	TorrentDownloadManager(const TorrentDownloadManager&);
	TorrentDownloadManager& operator=(const TorrentDownloadManager&);	
	
	void HandleAlerts();
	void UpdateDownloaders();
	
	void Run();
protected:
	TorrentDownloadManager();
	session& GetSession();
	void RegisterHandle(torrent_handle h,TorrentDownloader *td);
	TorrentDownloader* FindDownloader(torrent_handle h);
	void UnregisterHandle(torrent_handle h);
	void RemoveDownload(TorrentDownloader* t);
public:
	static TorrentDownloadManager& Instance();
	

	TorrentDownloader* NewTorrentDownloader();
	
	int GetDownloadThrottle();
	void SetDownloadThrottle(int throttle);	
	
	int GetUploadThrottle();
	void SetUploadThrottle(int throttle);	

	void Shutdown();
	
	bool ProvidesService(const string& service);

	virtual Downloader * Filter(const URL& url,map<string,string>& hints);
	
	friend class TorrentDownloader;

};

#endif
