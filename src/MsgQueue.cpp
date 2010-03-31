/*
    
    DownloadManager - http://www.excito.com/
    
    MsgQueue.cpp - this file is part of DownloadManager.
    
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
    
    $Id: MsgQueue.cpp 1183 2007-12-16 22:03:31Z tor $
*/

#include "MsgQueue.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif


MsgQueue::MsgQueue(string path,int token){
    struct stat sb;
    //int res;
    //key_t key;

    if (stat(path.c_str(),&sb)) {
        throw MsgError("Unable to stat");
    }

    if ((tok=ftok(path.c_str(),token))==-1) {
        throw MsgError("Unable to ftok");
    }
    /*
    if((key=msgget(tok,0660))>=0){
    	if((res=msgctl(key,IPC_RMID,NULL))<0){
    		perror("Failed to remove earlier queue");
    	}
    }
	*/
    if ((q_id=msgget(tok,IPC_CREAT|0660))==-1) {
        throw MsgError("Unable to get queue");
    }

}

void MsgQueue::SetGid(gid_t gid){
    struct msqid_ds msbuf;
    if(msgctl(q_id,IPC_STAT,&msbuf)){
        throw MsgError("Unable to stat queue");
    }
    msbuf.msg_perm.gid=gid;
    if(msgctl(q_id,IPC_SET,&msbuf)){
        throw MsgError("Unable set gid permissions");
    }

}

void MsgQueue::SetUid(uid_t uid){
    struct msqid_ds msbuf;
    if(msgctl(q_id,IPC_STAT,&msbuf)){
        throw MsgError("Unable to stat queue");
    }
    msbuf.msg_perm.uid=uid;
    if(msgctl(q_id,IPC_SET,&msbuf)){
        throw MsgError("Unable set uid permissions");
    }
}

void MsgQueue::SetPerm(unsigned short mode){
    struct msqid_ds msbuf;
    if(msgctl(q_id,IPC_STAT,&msbuf)){
        throw MsgError("Unable to stat queue");
    }
    msbuf.msg_perm.mode=mode;
    if(msgctl(q_id,IPC_SET,&msbuf)){
        throw MsgError("Unable set uid permissions");
    }
}

void MsgQueue::Send(const void* buf,unsigned int len, long type){

      void* msg=malloc(len+sizeof(long));

      if (msg==NULL) {
          throw MsgError("Malloc failed");
      }

      ((msgbuf*)msg)->mtype=type;
      memcpy(((msgbuf*)msg)->mtext,buf,len);

      if (msgsnd(q_id,msg,len,0)) {
          free(msg);
          throw MsgError("Unable to send message");
      }
      free(msg);

}

void MsgQueue::Recieve(void* buf,unsigned int len,long type){

    void* msg=malloc(len+sizeof(long));

    if (msg==NULL) {
        throw MsgError("Malloc failed");
    }

    if (msgrcv(q_id,msg,len,type,0)<0) {
        free(msg);
        throw MsgError("Unable to read message");
    }
    memcpy(buf,((msgbuf*)msg)->mtext,len);

    free(msg);

}


MsgQueue::~MsgQueue(){
}
