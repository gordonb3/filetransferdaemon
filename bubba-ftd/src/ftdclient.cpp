/*
    
    DownloadManager - http://www.excito.com/
    
    ftdclient.cpp - this file is part of DownloadManager.
    
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
    
    $Id: filetransferdaemon.cpp 1147 2007-11-12 18:35:34Z tor $
*/

#include <libeutils/Socket.h>
#include "Commands.h"
#include <libeutils/StringTools.h>
#include <libeutils/json/json.h>

#include <iostream>
#include <algorithm>
#include <syslog.h>
#include <popt.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>

using namespace std;

using namespace EUtils;

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

void SendJsonValue(const Json::Value& val, UnixClientStreamSocket& sock){
	Json::FastWriter writer;
	
	string r=writer.write(val);
	sock.Send(r.c_str(),r.length());
}

Json::Value ReadJsonValue(UnixClientStreamSocket& sock){
	Json::Value res;
	Json::Reader reader;

	string line=sock.ReadLine();
	if(line!=""){
		reader.parse(line,res);
	}
	return res;
}


int cancel_download(UnixClientStreamSocket& sock,const char *user, const char *uuid, const char *url){
	try{
		// Validate input
		if(! (user && (uuid || url))){
			cerr << "Invalid input to cancel"<<endl;
			return 1;
		}
		
		Json::Value cmd(Json::objectValue);
		cmd["cmd"]=CANCEL_DOWNLOAD;
		cmd["user"]=user;
		cmd["uuid"]="";
		cmd["url"]="";
		
		if(uuid){
			cmd["uuid"]=uuid;
		}
		
		if(url){
			cmd["url"]=url;
		}
		
		SendJsonValue(cmd,sock);
		
		Json::Value rep=ReadJsonValue(sock);	
		
		if(rep.isObject() && rep.isMember("cmd") && rep["cmd"].isIntegral()){
			int cmd=rep["cmd"].asInt();
			if(cmd==CMD_OK){
				return 0;
			}else if(cmd==CMD_FAIL){
				return 1;
			}else{
				cerr << "Unknown reply on cancel"<<endl;
			}
		}
	}catch(std::runtime_error* err){
		cout << "Caught exception "<< err->what()<<endl;
	}
	
	return 1;
}

void list_downloads(UnixClientStreamSocket& sock,const char *user){

	try{
		Json::Value cmd(Json::objectValue);
		cmd["cmd"]=LIST_DOWNLOADS;
		cmd["user"]=user;
		cmd["uuid"]="";
		
		SendJsonValue(cmd,sock);

		int repcmd=CMD_FAIL;
		do{
			Json::Value rep=ReadJsonValue(sock);	
		
			if(rep.isObject() && rep.isMember("cmd") && rep["cmd"].isIntegral()){
				repcmd=rep["cmd"].asInt();
				if(repcmd==LIST_DOWNLOADS){
					printValueTree(rep);
				}
			}else{
				if( !rep.isNull()){
					cout << "Unknown reply"<<endl;
					printValueTree(rep);
				}
				repcmd=CMD_FAIL;
			}
		}while(repcmd==LIST_DOWNLOADS);
	}catch(std::runtime_error* err){
		cout << "Caught exception "<< err->what()<<endl;
	}
}

int add_download(UnixClientStreamSocket& sock,const char *user, const char *url ){

	try{
		Json::Value cmd(Json::objectValue);
		cmd["cmd"]=ADD_DOWNLOAD;
		cmd["user"]=user;
		cmd["uuid"]=StringTools::SimpleUUID();
		cmd["url"]=url;
		cmd["policy"]=DLP_NONE;
		
		SendJsonValue(cmd,sock);
		
		Json::Value rep=ReadJsonValue(sock);	
		
		if(rep.isObject() && rep.isMember("cmd") && rep["cmd"].isIntegral()){
			int cmd=rep["cmd"].asInt();
			if(cmd==CMD_OK){
				return 0;
			}else if(cmd==CMD_FAIL){
				return 1;
			}else{
				cerr << "Unknown reply on cancel"<<endl;
			}
		}
	}catch(std::runtime_error* err){
		cout << "Caught exception "<< err->what()<<endl;
	}
	
	return 1;
}


int main(int argc, char** argv){
	
	poptContext optCon;
	char *uuid=NULL;
	char *user=NULL;
	char *url=NULL;
	int cancel=0;
	int list=0;
	int add=0;
	
	struct poptOption opts[]={
			{"uuid",'i',POPT_ARG_STRING,&uuid,0,"uuid to operate on",NULL},
			{"user",'u',POPT_ARG_STRING,&user,0,"User to operate with",NULL},
			{"cancel",'c',POPT_ARG_NONE,&cancel,0,"Cancel download",NULL},
			{"list",'l',POPT_ARG_NONE,&list,0,"List downloads by user",NULL},
			{"add",'a',POPT_ARG_NONE,&add,0,"Add [url] to download",NULL},
			{"url",'\0',POPT_ARG_STRING,&url,0,"Url to use",NULL},
			POPT_AUTOHELP
			{NULL,0,0,NULL,0}
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
	
	if(!list && !add && !cancel){
		fprintf(stderr,"Unknown  command");
		return 1;
	}
	try{
		UnixClientStreamSocket sock(SOCK_STREAM,"/tmp/ftdaemon");
		
		if(!sock.Connect()){
			cerr << "Could not connect to daemon"<<endl;
			return 1;
		}
		
		if(list){
			list_downloads(sock,user?user:(const char*)"tor");
			return 0;
		}
		
		if(add){
			if(url){
				add_download(sock, user?user:(const char*)"tor",url);
			}else{
				cerr << "Url missing"<<endl;
			}
		}
		
		if(cancel){
			
			cout<< "Cancel"<<endl;
			if(uuid || url){
				cancel_download(sock, user?user:(const char*)"tor",uuid,url);
			}else{
				cerr << "No uuid or url provided"<<endl;
			}
			return 0;
		}
	}catch(std::runtime_error* err){
		cerr << "Caught exception: "<<err->what()<<endl;
	}	
	
	return 0;
}
