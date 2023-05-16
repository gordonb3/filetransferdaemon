/*
    
    DownloadManager - http://www.excito.com/
    
    Downloader.h - this file is part of DownloadManager.
    
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
    
    $Id: Downloader.h 2144 2008-11-17 21:02:28Z tor $
*/

#ifndef DOWNLOADER_H
#define DOWNLOADER_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>
#include <list>
#include <sigc++/sigc++.h>
#include <libeutils/Url.h>
#include <libeutils/Condition.h>
#include <libeutils/json/json.h>

#define USER_AGENT "excito-ftd " PACKAGE_VERSION ";libtorrent"

using namespace EUtils;

using namespace std;

typedef void (*callback_t)(void*);
typedef map<callback_t,list<void*> > cb_map;

typedef enum my_status {
	NOTSTARTED,
	QUEUED,
	DOWNLOADED,
	CANCELINPROGRESS,
	CANCELDONE,
	INPROGRESS,
	FAILED
} dlstatus;

/*
 * Kepp track of notifications
 * OK - It's ok to add oneself for notification
 * CALLINPROGRESS - We are doing callbacks, too late
 * CALLDONE - We have done our callbacks, reap is ok.
 */
typedef enum {
	OK,
	CALLINPROGRESS,
	CALLDONE
} callbackstate;

class Downloader {
private:

	Downloader(const Downloader&);
	Downloader& operator=(const Downloader&);

	Mutex cbmutex;
	cb_map callbacks;


protected:
	EUtils::Condition completion;
	URL url;
	string destinationpath;
	string destinationname;
	Json::Value info;
	uid_t user;
	gid_t group;
	string uuid;
	double size,downloaded;
	dlstatus status;
	bool tip;
	bool cancel;
	Mutex reapmutex;
	callbackstate reapstatus;
	unsigned int policy;
	map<string,string> hints;


	void InvokeCallbacks();
	void Complete();

public:

	sigc::signal0<void> SignalDone;
	sigc::signal1<void, const std::string&> SignalFailed;

	/**
	 * Default constructor
	 * 
	 * @param url    Which url should be downloaded
	 * @param destination
	 *               Where should the resulting download be placed
	 * @param uuid   Unique id to identify this download.
	 */
	Downloader(string url="",string destinationpath="", string destinationname="", string uuid="");

	/**
	 * Setter for url attribute.
	 * 
	 * @param url    New url
	 */
	virtual void SetUrl(string url);
	/**
	 * Getter for url
	 * 
	 * @return current url
	 */
	virtual string GetUrl();

	/**
	 * Setter for userid to use
	 * 
	 * @param user   Username of file owner
	 */
	virtual void SetUser(uid_t user);
	/**
	 * Getter for user
	 * 
	 * @return 
	 */
	virtual uid_t GetUser();

	/**
	 * Setter for group id
	 * 
	 * @param group  Group which should own download
	 */
	virtual void SetGroup(gid_t group);
	/**
	 * Getter for group
	 * 
	 * @return group
	 */
	virtual gid_t GetGroup();

	/**
	 * Setter for destination, where shall download be stored.
	 * 
	 * @param dest   Destination path
	 */
	virtual void SetDestinationPath(string dest);

	/**
	 * Setter for _Suggested_ download name could be overridden 
	 * depending on which type of download being done.
	 * @param name name to be used for download
	 * 
	 */ 
	virtual void SetDownloadName(string name);

	/**
	 * Getter for destination
	 * 
	 * @return destination
	 */
	virtual string GetDestinationPath();

	/**
	 * Getter for name of download
	 * 
	 * @return name
	 */
	virtual string GetDownloadName();

	/**
	 * Getter for info of download
	 * 
	 * @return info
	 */
	virtual Json::Value GetInfo();

	/**
	 * Setter for info
	 * 
	 * @param info
	 */
	virtual void SetInfo(Json::Value);

	/**
	 * Setter for uuid
	 * 
	 * @param uuid
	 */
	virtual void SetUUID(string uuid);
	/**
	 * Getter for uuid
	 * 
	 * @return 
	 */
	virtual string GetUUID(void);

	/**
	 * Pure virtual start download. This is called to tell a downloader to
	 * start downloading.
	 * 
	 * Should be implemented by subclass and is not expected to block.
	 */
	virtual void StartDownload(void) {
	};

	/**
	 * Cancel currently running download.
	 */
	virtual void CancelDownload();

	/**
	 * Almost like cancel but here we should save any
	 * state for a possible restart
	 */
	virtual void StopDownload();
	
	/* Check if its ok to reap object */
	virtual bool ReapOK();
	
	/**
	 * Retrieve status from downloader object.
	 * 
	 * @return  dlstatus
	 * @see dlstatus
	 */
	virtual dlstatus GetStatus();

	/**
	 * Get total size of current download.
	 * 
	 * @return size in bytes
	 */
	virtual double Size();

	/**
	 * Data diwnloaded this far
	 * 
	 * @return size in bytes
	 */
	virtual double Downloaded();

	/**
	 * Block and wait for this download to complete.
	 */
	virtual void WaitForCompletion();

	/**
	 * Register callback that gets invoked when download done.
	 * 
	 * @param cb     Function that should be called
	 * @param data   Opaque data that gets passed back to callback when invoked
	 * @return true if register succesed. False otherwise.
	 */
	virtual bool RegisterCompletionCallback( callback_t cb,void* data);

	/**
	 * Virtual method which gets invoked everytime an implementation 
	 * calls UpdateProgress
	 */
	virtual void Progress();

	/**
	 * Updates statistics on download. Should be called periodically 
	 * by implementation.
	 * 
	 * @param dltotal Total download this far
	 * @param dlnow   Downloaded this far
	 * @param ultotal Uploaded total
	 * @param ulnow   Upload this far
	 * 
	 * @return continuation. If true returned implementation should cancel download.
	 */
	bool UpdateProgress(double dltotal,double dlnow, 
					double ultotal, double ulnow);


	/**
	 * This method is used to pass misc hints to the downloader. Most often
	 * gathered during the filter process selecting the apropriate downloader.
	 * 
	 * Usually the hints consists of headers provided by servers.
	 * 
	 * @param hints  Map with hints
	 */
	void SetHints(map<string,string> const& hints);
	
	/**
	 * Set policy for this download. 
	 * Could be things such as invicible, autoremove
	 * 
	 * @param p policy to set
	 */
	void SetPolicy(unsigned int p){ this->policy=p;}
	
	/**
	 * Retreives current policy for download
	 * 
	 * @return current policy
	 */
	
	unsigned int GetPolicy(void){return this->policy;}

	virtual ~Downloader();

};

class DownloadManager{
public:
	/**
	 * 
	 * Function to find out what a manager supports
	 * 
	 * @param service the service that is queried fx, torrent for bittorent
	 * 
	 * @return true if manager supports this false otherwise
	 * 
	 */
	virtual bool ProvidesService(const string& service);
	
	/**
	 * Get current download throttle
	 * 
	 * @return current setting or -1 if not throttled
	 */
	virtual int GetDownloadThrottle(){ return -1;}
	
	/**
	 * Set current download throttle
	 * 
	 * @param throttle new value for throttle
	 */
	virtual void SetDownloadThrottle(int throttle){ }	
	
	/**
	 * Get current upload throttle
	 * 
	 * @return current setting or -1 if not throttled
	 */
	virtual int GetUploadThrottle(){ return -1;}

	/**
	 * Set current upload throttle
	 * 
	 * @param throttle new value for throttle
	 */
	virtual void SetUploadThrottle(int throttle){}	
	
	
	
	/**
	 * Given an url returns a "newly" created download instance if supported
	 * 
	 * @param url url for download
	 * @param hints map with extra hints on mimetype etc. If new meta info is retreived it should be added for others to use.
	 * 
	 * @return an download instance if this manager supports this url NULL otherwise
	 */ 
	virtual Downloader* Filter(const URL &url,map<string,string>& hints)=0;
	virtual ~DownloadManager(){};
};

#endif
