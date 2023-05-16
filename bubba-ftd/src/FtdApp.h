/*
    
    DownloadManager - http://www.excito.com/
    
    FtdApp.h - this file is part of DownloadManager.
    
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
    
    $Id: FtdApp.h 2011 2008-10-13 21:05:02Z tor $
*/

#ifndef MY_FTDAPP_H
#define MY_FTDAPP_H

#include <map>
#include <list>
#include <string>

#include "Downloader.h"
#include <libeutils/Socket.h>
#include "Commands.h"
#include <libeutils/json/json.h>


using namespace std;

class FtdApp{

	Mutex transfermutex;
    map<string,list<Downloader*> > transfers;
    list<DownloadManager*> managers;
    Json::FastWriter writer;

    void ListUserDownloads(const string& user, UnixClientSocket* sock);
    bool FindByUUID(string& uuid, string& user);
    bool FindByURL(string& url);
    void RemoveDownload(Downloader* dl);
    static void Remove(void* dl);
    static void DeferredRemove(void* dl);
    Downloader* FindDownloader(const URL &url);
    DownloadManager* FindManagerByService(const string& service);
    void SendDownload(const string& user, Downloader* dl,UnixClientSocket* sock);
    void SendJsonValue(Json::Value& val, UnixClientSocket* sock);
    void dlDone(void* downloader);
    
protected:
	FtdApp();
	FtdApp(const FtdApp&);
	FtdApp& operator=(const FtdApp&);
public:

	static FtdApp& Instance();

    void AddDownload(Json::Value& req, UnixClientSocket* sock);
    void RegisterDownload(const string& user, Downloader* dl);
    void DeRegisterDownload(const string& user, Downloader* dl);
    string GetDownloadDir(const string& user);
    void GetDownloadThrottle(Json::Value& req, UnixClientSocket* sock);
    void SetDownloadThrottle(Json::Value& req, UnixClientSocket* sock);
    void GetUploadThrottle(Json::Value& req, UnixClientSocket* sock);
    void SetUploadThrottle(Json::Value& req, UnixClientSocket* sock);

    void CancelDownload(Json::Value& req);
    void ListDownloads(Json::Value& req, UnixClientSocket* sock);
    void GetDownloadByUUID(string& user, string& uuid, UnixClientSocket* sock);
    void SendOk(UnixClientSocket* sock);
    void SendFail(UnixClientSocket* sock);
    void AddFilter(DownloadManager* mgr);
    
    void ShutDown();

    virtual ~FtdApp();

};

#endif
