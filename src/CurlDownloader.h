/*
    
    DownloadManager - http://www.excito.com/
    
    CurlDownloader.h - this file is part of DownloadManager.
    
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
    
    $Id: CurlDownloader.h 2144 2008-11-17 21:02:28Z tor $
*/

#ifndef MY_CURLDOWNLOADER
#define MY_CURLDOWNLOADER

#include <libeutils/Thread.h>
#include "Downloader.h"
#include <libeutils/Url.h>
#include <libeutils/Mutex.h>
#include <curl/curl.h>
#include <iosfwd>
#include <map>

using namespace EUtils;

class CurlDownloader: public Downloader, public Thread {
private:
	CurlDownloader(const CurlDownloader&);
	CurlDownloader& operator=(const CurlDownloader&);

	unsigned int m_timeout; // Transaction timeout
	unsigned int m_ctimeout; // Connect timeout
	string m_useragent;
	
	Mutex curllock;
	CURL *curl;
	
	bool mydest; //Do we own this dest?
	bool headersonly; // Only retreive headers?
	map<string,string> headers;
	iostream *dest;
	void Run();
	static size_t WriteContent(void* data, size_t size, size_t nmemb, void* handle);
	static size_t Headers(void *data, size_t size, size_t nmemb, void *handle);

protected:
	static int CbProgress(void* client, double dltotal, 
					  double dlnow, double ultotal, double ulnow);

	CurlDownloader(string url="",string destination="",string uuid="");
	

public:


	void SetStream(iostream *str);
	iostream* GetStream() {
		return dest;
	};

	void SetTimeout(unsigned int to){m_timeout=to;}
	void SetConnectTimeout(unsigned int to){m_ctimeout=to;}
	void SetUserAgent(const string agent);
	void SetHeadersOnly(bool which);
	map<string,string> GetHeaders();
	virtual void StartDownload(void);
	virtual void StopDownload(void);


	virtual ~CurlDownloader();

	friend class CurlDownloadManager;
	friend class HttpDownloader;

};


class CurlDownloadManager: public DownloadManager {
	CurlDownloadManager();
public:
	static CurlDownloadManager& Instance();
	CurlDownloader* GetDownloader(string url="",string destination="",string uuid="");
	Downloader* Filter(const URL &url,map<string,string>& hints);
	bool ProvidesService(const string& service);
	
};

#endif
