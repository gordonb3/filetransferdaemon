/*
    
    DownloadManager - http://www.excito.com/
    
    filetransferdaemon.cpp - this file is part of DownloadManager.
    
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
    
    $Id: filetransferdaemon.cpp 2011 2008-10-13 21:05:02Z tor $
*/

#include "FtdConfig.h"
#include "Downloader.h"
#include "CurlDownloader.h"
#include "TorrentDownloader.h"
#include "SocketFdDownloader.h"
#include <libeutils/Socket.h>
#include "Commands.h"
#include "FtdApp.h"
#include <libeutils/FileUtils.h>
#include <libeutils/json/json.h>

#include <stdio.h>
#include <string.h>
#include <grp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <popt.h>
#include <sigc++/sigc++.h>
#include <signal.h>


#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <list>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

using namespace std;

static const char* appversion=PACKAGE_VERSION;
static const char* builddate=__DATE__ " " __TIME__;
static volatile int do_shutdown=0;

#ifdef DEBUG
static void
printValueTree(Json::Value &value, const std::string &path = "." )
{
   switch ( value.type() )
   {
   case Json::nullValue:
      printf("%s=null\n", path.c_str() );
      break;
   case Json::intValue:
      printf("%s=%d\n", path.c_str(), value.asInt() );
      break;
   case Json::uintValue:
      printf("%s=%u\n", path.c_str(), value.asUInt() );
      break;
   case Json::realValue:
      printf("%s=%.16g\n", path.c_str(), value.asDouble() );
      break;
   case Json::stringValue:
      printf("%s=\"%s\"\n", path.c_str(), value.asString().c_str() );
      break;
   case Json::booleanValue:
      printf("%s=%s\n", path.c_str(), value.asBool() ? "true" : "false" );
      break;
   case Json::arrayValue:
      {
         printf("%s=[]\n", path.c_str() );
         int size = value.size();
         for ( int index =0; index < size; ++index )
         {
            static char buffer[16];
            sprintf( buffer, "[%d]", index );
            printValueTree(value[index], path + buffer );
         }
      }
      break;
   case Json::objectValue:
      {
         printf("%s={}\n", path.c_str() );
         Json::Value::Members members( value.getMemberNames() );
         std::sort( members.begin(), members.end() );
         std::string suffix = *(path.end()-1) == '.' ? "" : ".";
         for ( Json::Value::Members::iterator it = members.begin(); 
               it != members.end(); 
               ++it )
         {
            const std::string &name = *it;
            printValueTree(value[name], path + suffix + name );
         }
      }
      break;
   default:
      break;
   }
}
#endif

void sighandler(int signum){
	if (signum==SIGINT || signum==SIGTERM) {
		// Controlled shutdown
		do_shutdown=1;			
	}
}

int main(int argc,char** argv){
	int daemonize=0;
	int version=0;
	const char* group="www-data";
	openlog("ftd",LOG_PERROR,LOG_DAEMON);
	// Set logmask, default is LOG_NOTICE (5)
	setlogmask(LOG_UPTO( FtdConfig::Instance().GetIntegerOrDefault("general","loglevel",5)));

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sighandler);
	signal(SIGTERM,sighandler);

	try {

		poptContext optCon;
		struct poptOption opts[]={
			{"fg",'f',POPT_ARG_NONE,&daemonize,0,"Run in foreground, dont daemonize",NULL},
			{"version",'v',POPT_ARG_NONE,&version,0,"Show version",NULL},
			{"group",'g',POPT_ARG_STRING,&group,0,"Group allowed to talk to ftd","group"},
			POPT_AUTOHELP
			{ NULL, 0, 0, NULL, 0}
		};

		optCon=poptGetContext(NULL,argc,(const char**)argv,opts,0);
		int rc = poptGetNextOpt(optCon);

		if (rc<-1) {
			fprintf(stderr, "%s: %s\n",
					poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
					poptStrerror(rc));
			return 1;
		}

		poptFreeContext(optCon);

		if (version) {
			cerr << "Version: "<< appversion<<endl;
			cerr << "Built  : "<< builddate<<endl;
			return 0;
		}

		if (getuid()!=0) {
			syslog(LOG_ERR, "Started as non root terminating");
			cerr << "You must be root to run application"<<endl;
			return 1;
		}

		syslog(LOG_INFO,"Started");

		if (!daemonize) {
			daemon(1,0);
			syslog(LOG_INFO,"Daemonizing");
			ofstream pidfile("/var/run/ftd.pid");
			pidfile<<getpid()<<endl;
			pidfile.close();
		}

		unlink("/tmp/ftdaemon");

		UnixServerSocket sock(SOCK_STREAM,"/tmp/ftdaemon");
		chmod("/tmp/ftdaemon",0660);
		FileUtils::Chown("/tmp/ftdaemon","root",group);

		FtdApp& fapp=FtdApp::Instance();
		// Order is important here both curl and torrent downloader acccept the same scheme
		fapp.AddFilter(&SocketFdDownloadManager::Instance());
		fapp.AddFilter(&CurlDownloadManager::Instance());
		fapp.AddFilter(&TorrentDownloadManager::Instance());

		Json::Reader reader;
		do {
			try{
			
				UnixClientSocket* con=sock.Accept();
				
				if(!con){
					continue;
				}

				char buf[16384];
				ssize_t r=con->Receive(buf, 16384);
				if(r>0){
					buf[r]=0;
					Json::Value res;
					bool success=reader.parse(buf,res);
					if(!success){
						syslog(LOG_ERR,"No valid request");
						fapp.SendFail(con);
						delete con;
						continue;
					}
					if(!res.isMember("cmd")){
						syslog(LOG_ERR,"Request missing command");
						fapp.SendFail(con);
						delete con;
						continue;
					}
					if(!res["cmd"].isInt()){
						syslog(LOG_ERR,"Command illegal format");
						fapp.SendFail(con);
						delete con;
						continue;
					}
#ifdef DEBUG
					printValueTree(res);
#endif
					switch(res["cmd"].asInt()){
					case ADD_DOWNLOAD:
						fapp.AddDownload(res,con);
						break;
					case CANCEL_DOWNLOAD:
						fapp.CancelDownload(res);
						break;
					case LIST_DOWNLOADS:
						fapp.ListDownloads(res,con);
						break;
					case GET_DOWNLOAD_THROTTLE:
						fapp.GetDownloadThrottle(res,con);
						break;
					case SET_DOWNLOAD_THROTTLE:
						fapp.SetDownloadThrottle(res,con);
						break;
					case GET_UPLOAD_THROTTLE:
						fapp.GetUploadThrottle(res,con);
						break;
					case SET_UPLOAD_THROTTLE:
						fapp.SetUploadThrottle(res,con);
						break;
					case CMD_SHUTDOWN:
						do_shutdown=true;
						fapp.SendOk(con);
						break;
					default:
						fapp.SendFail(con);
						break;
					}
						
				}
				delete con;
			}catch(std::runtime_error* err){
				syslog(LOG_NOTICE,"Caught exception waiting for message: [%s]",err->what());
			}

		} while ( !do_shutdown);

	} catch (std::runtime_error* err) {
		cout << "Caught exception:"<<err->what()<<endl;
		return 1;
	}

	if(do_shutdown){
		syslog(LOG_NOTICE,"shutting down");
		FtdApp::Instance().ShutDown();
		TorrentDownloadManager::Instance().Shutdown();
		syslog(LOG_NOTICE,"terminating");
	}
    
	return 0;
}
