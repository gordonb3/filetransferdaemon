/*

   DownloadManager - http://www.excito.com/

   Upload.cpp - this file is part of DownloadManager.

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

   $Id: upload.cpp 2011 2008-10-13 21:05:02Z tor $
   */

#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <libeutils/Socket.h>
#include "Commands.h"
#include <libeutils/ECGI.h>
#include "FtdConfig.h"
#include <libeutils/PHPSession.h>
#include <libeutils/json/json.h>
#include <libeutils/StringTools.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <syslog.h>
#include <inttypes.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif


extern char **environ;

using namespace std;

void show_environ(map<string,string>& env){
	map<string,string>::const_iterator eIt=env.begin();
	for(;eIt!=env.end();++eIt){
		pair<string,string> elem=*eIt;
		cout<<"Env: ["<<elem.first<<"] = ["<< elem.second<<"]<br/>"<<endl;
	}

}

void parse_environ(map<string,string> &env){
	char **iter=environ;
	int index;
	while(*iter){
		string row(*iter);
		index=row.find("=");
		env[row.substr(0,index)]=row.substr(index+1,row.size()-index);
		iter++;
	}
}

static void SendJsonValue(const Json::Value& val, UnixClientSocket& sock){
	Json::FastWriter writer;

	string r=writer.write(val);
	sock.Send(r.c_str(),r.length());
}

static Json::Value ReadJsonValue(UnixClientSocket& sock){
	Json::Value res;
	Json::Reader reader;
	char buf[16384];
	ssize_t r=sock.Receive(buf, 16384);
	if(r>0){
		reader.parse(buf,res);
	}
	return res;
}



int main (int argc, char *argv[]) {

//	cout << "Content-type: application/json"<<endl<<endl;
	cout << "Content-type: text/plain"<<endl<<endl;
	Json::Value res;
	res["result"]=true;
#ifdef DEBUG
	map<string,string> env;
	parse_environ(env);
	show_environ(env);
#endif

	try{
		ECGI cgi;

		cgi.parseCookies();

		if(cgi.cookie("PHPSESSID")==""){
			res["result"]=false;
			res["what"]="No session";
		}else{
			PHPSession session(FtdConfig::Instance().GetStringOrDefault("general","phpsession","/var/lib/php4/sess_")+cgi.cookie("PHPSESSID"));

#ifdef DEBUG
			cout << "Session: "<<session["valid"]<<" user: "<<session["user"]<<endl;
#endif

			if(session["valid"]=="1" && session["user"]!=""){
				try {

					if (!cgi.isMulti()) {
						res["result"]=false;
						res["what"]="Missing multipart post (This is no upload)";
					}else{

						char tmnam[100];
						sprintf(tmnam,"/tmp/buplXXXXXX");
						int tmpfd;
						if (!(tmpfd=mkstemp(tmnam))) {
							return -1;
						}
						// Uggly hack, we only want the unique filename to pass on
						close(tmpfd);
						unlink(tmnam);
#ifdef DEBUG
						cout << "Tempname:["<<tmnam<<"]<br/>"<<endl;
#endif
						UnixServerSocket sock(SOCK_STREAM,tmnam);

						UnixClientSocket csock(SOCK_STREAM,"/tmp/ftdaemon");

						if(!csock.Connect()){
							throw new runtime_error("Unable to connect to server daemon");
						}

						Json::Value cmd(Json::objectValue);
						cmd["cmd"]=ADD_DOWNLOAD;
						cmd["uuid"]=StringTools::SimpleUUID();
						cmd["policy"]=DLP_AUTOREMOVE|DLP_HIDDEN;;
						cmd["user"]=session["user"];
						cmd["url"]=string("upload:///")+tmnam;
						SendJsonValue(cmd,csock);

						Json::Value rep=ReadJsonValue(csock);

						if(rep.isObject() && rep.isMember("cmd") && rep["cmd"].isIntegral()){
							int cmd=rep["cmd"].asInt();
							if(cmd!=CMD_OK){
								throw new runtime_error("Send upload command failed");
							}else{
								UnixClientSocket* client=sock.Accept();

								client->SendFd(fileno(stdin));
								char rbuf[100];
								client->Receive(rbuf,sizeof(rbuf));

								if (rbuf[0]!='!') {
									delete(client);
									throw new runtime_error("Transfer of fd failed");
								}

								client->Send(cgi.getBoundary().c_str(),cgi.getBoundary().size());

								client->Receive(rbuf,sizeof(rbuf));

								if (rbuf[0]!='!') {
									delete(client);
									throw new runtime_error("Transfer of boundary failed");
								}

								sprintf(rbuf,"%" PRIu64,cgi.getContentLength());
								client->Send(rbuf,strlen(rbuf));

								client->Receive(rbuf,sizeof(rbuf));

								if (rbuf[0]!='!') {
									delete(client);
									throw new runtime_error("Transfer of content length failed");
								}

								// Wait for completion
								client->Receive(rbuf,sizeof(rbuf));

#ifdef DEBUG
								cout<<"Got answer:"<<string(rbuf)<<endl;
#endif
								delete(client);

							}
						}
					}

				} catch ( std::runtime_error* e ) {
					res["result"]=false;
					stringstream ss;
					ss <<"Caught runtime exception ["<<e->what()<<"]";
					res["what"]=ss.str();
				}
			}else{
				res["result"]=false;
				res["what"]="Not logged in";
			}
		}
	}catch(std::runtime_error& e){
		res["result"]=false;
		stringstream ss;
		ss << "Caught runtime exception(2) ["<<e.what()<<"]";
		res["what"]=ss.str();
	}
	Json::FastWriter writer;
	cout << writer.write(res)<<endl;

	return(0);
}

