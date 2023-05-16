/*

    libeutils - http://www.excito.com/

    Socket.cpp - this file is part of libeutils.

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

#include "Socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

namespace EUtils{

union fdmsg {
        struct cmsghdr h;
        char buf[CMSG_SPACE(sizeof(int))];
};

/*

   	Implementatoion of UnixSocket

*/

UnixSocket::UnixSocket(int type, string path){

	this->socktype=type;
	this->path=path;

	if((this->sock=socket(PF_UNIX,type,0))<0){
		throw new std::runtime_error("Failed to create socket: "+string(strerror(errno)));
	}
}

UnixSocket::~UnixSocket(){
	close(this->sock);
}

int UnixSocket::getSocketFd(){
	return this->sock;
}


/*

   	Implementatoion of UnixClientSocket

*/


UnixClientSocket::UnixClientSocket(int type, string path):UnixSocket(type,path){

	memset(&this->addr,0,sizeof(struct sockaddr_un));
	this->addr.sun_family = AF_UNIX;
	strncpy(this->addr.sun_path,path.c_str(),path.size());


}
bool UnixClientSocket::Connect(){
	if (connect(this->sock,(struct sockaddr *) &this->addr,sizeof(struct sockaddr_un)) == -1) {
		return false;
	}
	return true;
}

size_t UnixClientSocket::Send(const void *buf, size_t len){
	size_t ret;
	if ((ret=send(this->sock,buf,len,0))<0) {
		throw new std::runtime_error("Unable to send data: "+string(strerror(errno)));
	}
	return ret;
}

size_t UnixClientSocket::Receive(void *buf, size_t len){
	size_t ret;

	if ((ret=recv(this->sock,buf,len,0))<0) {
		throw new std::runtime_error("Unable to receive data: "+string(strerror(errno)));
	}
	return ret;
}

void UnixClientSocket::SendFd(int fd) {
	int ret = 0;
	struct iovec  iov[1];
	struct msghdr msg;

	iov[0].iov_base = &ret;
	iov[0].iov_len  = 1;

	msg.msg_iov     = iov;
	msg.msg_iovlen  = 1;
	msg.msg_name    = 0;
	msg.msg_namelen = 0;

	union  fdmsg  cmsg;
	struct cmsghdr* h;

	msg.msg_control = cmsg.buf;
	msg.msg_controllen = sizeof(union fdmsg);
	msg.msg_flags = 0;

	h = CMSG_FIRSTHDR(&msg);
	h->cmsg_len   = CMSG_LEN(sizeof(int));
	h->cmsg_level = SOL_SOCKET;
	h->cmsg_type  = SCM_RIGHTS;
	*((int*)CMSG_DATA(h)) = fd;

	if ( sendmsg(this->sock, &msg, 0) < 0 ) {
		throw new std::runtime_error("Unable to send fd:"+string(strerror(errno)));
	}
}

int UnixClientSocket::ReceiveFd() {
	int count;
	int ret = 0;
	struct iovec  iov[1];
	struct msghdr msg;

	iov[0].iov_base = &ret;	/* don't receive any data */
	iov[0].iov_len  = 1;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	union fdmsg  cmsg;
	struct cmsghdr* h;

	msg.msg_control = cmsg.buf;
	msg.msg_controllen = sizeof(union fdmsg);
	msg.msg_flags = 0;

	h = CMSG_FIRSTHDR(&msg);
	h->cmsg_len   = CMSG_LEN(sizeof(int));
	h->cmsg_level = SOL_SOCKET;  /* Linux does not set these */
	h->cmsg_type  = SCM_RIGHTS;  /* upon return */
	*((int*)CMSG_DATA(h)) = -1;

	if ( (count = recvmsg(this->sock, &msg, 0)) < 0 ) {
		throw new std::runtime_error("Unable to receive message for fd:"+string(strerror(errno)));
	} else {
		h = CMSG_FIRSTHDR(&msg);	  /* can realloc? */
		if ( h == NULL
			|| h->cmsg_len    != CMSG_LEN(sizeof(int))
			|| h->cmsg_level  != SOL_SOCKET
			|| h->cmsg_type   != SCM_RIGHTS ) {
			/* This should really never happen */
			if ( h )
				throw new std::runtime_error("Protocol failure");
			else
				throw new std::runtime_error("Protocol failure (NULL)");
		} else {
			ret = *((int*)CMSG_DATA(h));
		}
	}

	return ret;
}

UnixClientSocket::~UnixClientSocket(){
	close(this->sock);
}

/*

   	Implementatoion of UnixClientStreamSocket

*/

string UnixClientStreamSocket::ReadLine(void){
	char buf[16384];

	if(fgets(buf,16384,this->fil)){
		return string(buf);
	}else{
		if(ferror(this->fil)){
			throw new std::runtime_error("Readline errror");
		}
		return "";
	}
}

void UnixClientStreamSocket::WriteLine(const string& line){
	this->Send(line.c_str(),line.length());
}

size_t UnixClientStreamSocket::Receive(void* buf,size_t len){
	return fread(buf,len,1,this->fil);
}

size_t UnixClientStreamSocket::Send(const void* buf, size_t len){
	return fwrite(buf,len,1,this->fil);

}

UnixClientStreamSocket::~UnixClientStreamSocket(){
	fclose(this->fil);
}

/*

   	Implementatoion of UnixServerSocket

*/

#include <unistd.h>

UnixServerSocket::UnixServerSocket(int type, string path):UnixSocket(type,path){
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path,path.c_str(),path.size());

	if (bind(this->sock,(struct sockaddr *) &addr,sizeof(struct sockaddr_un)) == -1) {
		throw new std::runtime_error("Unable to bind to adress: "+string(strerror(errno)));
	}
	this->timeout.tv_sec=0;
	this->timeout.tv_usec=0;
	this->timedout=false;
}

void UnixServerSocket::SetTimeout(long sec, long usec){
	this->timeout.tv_sec=sec;
	this->timeout.tv_usec=usec;
}

bool UnixServerSocket::TimedOut(){
	return this->timedout;
}

UnixClientSocket* UnixServerSocket::Accept(){

	if(listen(this->sock,5)<0){
		throw new std::runtime_error("Failed to listen for connection");
	}
	struct timeval* timer=NULL;
	struct timeval timercp;
	if(timeout.tv_sec>0 || timeout.tv_usec>0){
		// Use a copy of our timeval since it will be updated by system
		timercp=this->timeout;
		timer=&timercp;
	}

	fd_set r_set;
	int selret;
	FD_ZERO(&r_set);
	FD_SET(this->sock,&r_set);
	if((selret=select(this->sock+1,&r_set,NULL,NULL,timer)>0)){
		int clientsock;
		struct sockaddr_un addr;
		socklen_t addr_len=sizeof(sockaddr_un);
		if((clientsock=accept(this->sock,(struct sockaddr *)&addr, &addr_len))<0){
			throw new std::runtime_error("Failed to accept new connection:"+string(strerror(errno)));
		}
		return new UnixClientSocket(clientsock);
	}else{
		this->timedout=selret==0;
		return NULL;
	}
}

UnixServerSocket::~UnixServerSocket(){
	if (this->path!="") {
		unlink(this->path.c_str());
	}
}
}
