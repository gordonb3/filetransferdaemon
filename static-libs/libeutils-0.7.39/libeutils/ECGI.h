/*

    libeutils - http://www.excito.com/

    ECGI.h - this file is part of libeutils.

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

#ifndef ECGI_H_
#define ECGI_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>
#include <list>
#include <stdexcept>
#include <unistd.h>
#include <stdint.h>
#include <sigc++/sigc++.h>

using namespace std;

namespace EUtils{

#define BOUNDARY_MARGIN 128


/**
 * State that process state can be in
 */

enum State{
	EMPTY,
	STARTOFFIELD,
	INFIELD,
	END
};

/**
 * Result return value from processchunk
 */

enum ProcRes{
	ERROR=-1,
	DONE=0,
	FULLFIELD,
	PARTIALFIELD,
	FULLFILE,
	PARTIALFILE
};

class ECGI
{
public:
	ECGI(int fd=STDIN_FILENO);
	virtual ~ECGI();

	// Signals
	sigc::signal1<void, const std::string&> FieldAdded;
	sigc::signal1<void, const std::string&> FileUploaded;


	void setBoundary(string boundary){this->boundary=boundary;}
	string& getBoundary(void){return this->boundary;}

	void setContentLength(int64_t len){this->content_length=len;}
	int64_t getContentLength(void){return this->content_length;}

	string& getUploadDirectory(){return this->upload_directory;}
	void setUploadDirectory(string path){this->upload_directory=path;}

	void setIsMulti(bool m){this->ismulti=m;}
	bool isMulti(void){return this->ismulti;}

	void setFileDescriptor(int fd){this->fd=fd;}

	void parseCookies(void);

	string cookie(string key){return this->cookies[key];}

	string field(string key){return this->var[key];}

	map<string,string>& fileNames(){return this->filenames;}

	string& getCurrentFilename(void){
		return this->cur_filename;
	}

	int64_t getDownloaded(void){return this->content_read;}

	int64_t getDownloadSize(void){return this->content_length;}

	void cancelUpload(){this->cancel=true;}
	bool canceled(){return this->cancel;}

	// Process post
	void parse(void);

private:

	// File descriptor to read content from
	int fd;

	// Flag telling us if stream is multipart or not.
	bool ismulti;
	// Parsed environment
	map<string,string> env;

	// Parsed fields in post
	map<string,string> var;

	// Cookies
	map<string,string> cookies;

	// Filename mapping
	map<string,string> filenames;

	// Upload directory to save files in
	string upload_directory;

	// Length of content
	int64_t content_length;

	// How much of content is read
	int64_t content_read;

	// Readbuffercontent_length
	unsigned char* readbuf;

	// Size of readbuf
	ssize_t bufsize;

	// Current read position in buffer
	unsigned char* cur_read;

	// How much valid data in buffer. Relative to cur_read
	ssize_t  buf_fill;

	// How much can we safely read out of buffer without trashing any vital info.
	ssize_t max_read;

	// Which part are we working on
	int part;

	// Current fieldname
	string cur_fieldname;

	// Current filename;
	string cur_filename;

	// Boundary for multipart posts
	string boundary;

	// Process state
	enum State state;

	// Last result from process chunk
	int last_result;

	/**
	 * FInd next boundary in buffer
	 */
	
	 unsigned char* findBoundary();
	
	/**
	 * Parse multipart stream
	 */
	
	 void processMultiPart(void);

	 /**
	  *  Parse non multipart stream (GET and POST)
	  */
	 void processNonMultiPart(void);

	 /**
	  * Non multi action parse fields from regular post/get
	 */
	 void parseFields(string s);
	
	/**
	 * Process chunk, read data and find parts
	 */
	int processChunk();

	/**
	 * Read environment into map
	 */
	void parseEnv();

	/**
	 * Fill buffer from fd
	 */
	ssize_t readPostPart();

	/**
	 * Read one line from buf
	 */
	unsigned char* readString();

	/**
	 * Advance readpointer and bufsize
	 */
	 void advancePointer(ssize_t len);
	
	 /**
	  * Fix correct filename Internet
	  * explorer sends complete path to file.
	  * This function returns last part ie the filename
	  */
	 string getFileName(string s);
	
	 bool cancel;

};

}
#endif /*ECGI_H_*/
