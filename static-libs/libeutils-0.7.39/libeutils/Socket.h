/*

    libeutils - http://www.excito.com/

    Socket.h - this file is part of libeutils.

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

#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>


#include <string>
#include <stdexcept>

using namespace std;

namespace EUtils{

class UnixSocket{
protected:
	int socktype;
	int sock;
	string path;
public:
	UnixSocket(int type,string path);
	UnixSocket(int sockfd):sock(sockfd){}
	int getSocketFd();

	virtual ~UnixSocket();
};

class UnixClientSocket: public UnixSocket{
	struct sockaddr_un addr;
public:
	UnixClientSocket(int type,string path);
	UnixClientSocket(int sockfd):UnixSocket(sockfd){};
	bool Connect();
	void SendFd(int fd);
	int ReceiveFd();

	virtual size_t Send(const void* buf, size_t len);
	virtual size_t Receive(void *buf, size_t len);

	virtual ~UnixClientSocket();
};

class UnixClientStreamSocket: public UnixClientSocket{
	FILE* fil;
	void SendFd(int fd);
	int ReceiveFd();
public:
	UnixClientStreamSocket(int type,string path):UnixClientSocket(type,path){
		this->fil=fdopen(sock,"r+");
	}

	UnixClientStreamSocket(int sockfd):UnixClientSocket(sockfd){
		this->fil=fdopen(sock,"r+");
	}

	virtual size_t Send(const void* buf, size_t len);
	virtual size_t Receive(void *buf, size_t len);
	string ReadLine(void);
	void WriteLine(const string& line);

	virtual ~UnixClientStreamSocket();

};

class UnixServerSocket: public UnixSocket{
	struct timeval timeout;
	bool timedout;
public:
	UnixServerSocket(int type, string path);

	void SetTimeout(long sec, long usec);
	bool TimedOut();

	UnixClientSocket* Accept();

	virtual ~UnixServerSocket();
};
}
#endif
