/*
    
    DownloadManager - http://www.excito.com/
    
    SocketFdDownloader.h - this file is part of DownloadManager.
    
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
    
    $Id: SocketFdDownloader.h 2015 2008-10-14 13:45:35Z tor $
*/

#ifndef MY_SOCKETFDDOWNLOADER_H
#define MY_SOCKETFDDOWNLOADER_H

#include "Downloader.h"
#include <boost/thread.hpp>
#include <libeutils/Url.h>
#include <libeutils/ECGI.h>

class SocketFdDownloadManager;

/**
 * This downloader is intended for use with upload functionality.
 * 
 * Basic operation is as follows.
 * 
 * URL accepted is upload://path.to.socket The downloader then opens 
 * this socket in client mode. Receives the FD from the CGI script and
 * starts to parse the RFC1867 post creating files as needed. 
 * 
 * @author "Tor Krill" <tor@excito.com>
 */
class SocketFdDownloader: public Downloader {
private:
	boost::thread    m_Thread;

	/**
	 * File descriptor received over socket.
	 */
	int fd; 

	/**
	 * CGI object to retreive files
	 */
	ECGI cgi;

	/**
	 * Move files in correct place and 
	 * set correct owner on uploaded files
	 */
	void FixFiles();

	/**
	 * Cleanup after a failed upload
	 * Delete tempfiles and socket
	 */
	void CleanUp();
	/**
	 * Reference to manager
	 */
	SocketFdDownloadManager* mgr;

	/**
	 * Hidden copy constructor
	 */
	SocketFdDownloader(const SocketFdDownloader&);
	/**
	 * Dont allow copying.
	 * 
	 * @return 
	 */
	SocketFdDownloader& operator=(const SocketFdDownloader&);
	
	/**
	 * Signal callback for setting uuid if found in upload.
	 */
	void FilterFields(const string& field);

protected:
	/**
	 * Hidden constructor. Only manager can create instances of us.
	 * 
	 * @param manager
	 */
	SocketFdDownloader(SocketFdDownloadManager* manager):fd(0),mgr(manager){};

	/**
	 * Pure virtual start download. This is called to tell a downloader to
	 * start downloading.
	 *
	 * Should be implemented by subclass and is not expected to block.
	 */
public:
	/**
	 * Start upload
	 */
	virtual void StartDownload(void);
	
	/**
	 * Stop current download
	 */
	virtual void StopDownload(void);

	/**
	 * Thread run Method
	 */
	virtual void Run();
	
	/* 
	 *  Overridden methods from download
	 */

	/**
	 * Get total size of current download.
	 * 
	 * @return size in bytes
	 */
	 
	virtual double Size(){ return cgi.getDownloadSize();}

	/**
	 * Data downloaded this far
	 * 
	 * @return size in bytes
	 */	 
	virtual double Downloaded(){ return cgi.getDownloaded();}

	/**
	 * Getter for info of download
	 * 
	 * @return info
	 */	
	virtual Json::Value GetInfo(){
		Json::Value ret(Json::objectValue);
		ret["type"]="upload";
		ret["name"]= cgi.getCurrentFilename();
		return ret;
	}

	/**
	 * Cancel currently running download.
	 */
	virtual void CancelDownload();


	/**
	 * Destructor
	 */
	virtual ~SocketFdDownloader();

	friend class SocketFdDownloadManager;
};

class SocketFdDownloadManager: public DownloadManager {
	SocketFdDownloadManager();
public:
	static SocketFdDownloadManager& Instance();
	SocketFdDownloader* NewSocketFdDownloader();
	Downloader* Filter(const URL &url,map<string,string>& hints); 
};


#endif
