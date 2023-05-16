/*

    libeutils - http://www.excito.com/

    SysvMsgQueue.cpp - this file is part of libeutils.

    Copyright (C) 2009 Tor Krill <tor@excito.com>

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

#include "SysvMsgQueue.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>

namespace EUtils {

SysvMsgQueue::SysvMsgQueue(const string& path, int token):SysvIPC(path,token) {

	if ((q_id=msgget(tok,IPC_CREAT|0660))==-1) {
        throw IpcError("Unable to get queue");
    }

}

void SysvMsgQueue::SetGid(gid_t gid){
    struct msqid_ds msbuf;

    if(msgctl(q_id,IPC_STAT,&msbuf)){
        throw IpcError("Unable to stat queue");
    }
    msbuf.msg_perm.gid=gid;
    if(msgctl(q_id,IPC_SET,&msbuf)){
        throw IpcError("Unable set gid permissions");
    }

}

void SysvMsgQueue::SetUid(uid_t uid){
    struct msqid_ds msbuf;
    if(msgctl(q_id,IPC_STAT,&msbuf)){
        throw IpcError("Unable to stat queue");
    }
    msbuf.msg_perm.uid=uid;
    if(msgctl(q_id,IPC_SET,&msbuf)){
        throw IpcError("Unable set uid permissions");
    }
}

void SysvMsgQueue::SetPerm(unsigned short mode){
    struct msqid_ds msbuf;
    if(msgctl(q_id,IPC_STAT,&msbuf)){
        throw IpcError("Unable to stat queue");
    }
    msbuf.msg_perm.mode=mode;
    if(msgctl(q_id,IPC_SET,&msbuf)){
        throw IpcError("Unable set uid permissions");
    }
}

void SysvMsgQueue::Send(const void* buf,unsigned int len, long type){

      void* msg=malloc(len+sizeof(long));

      if (msg==NULL) {
          throw IpcError("Malloc failed");
      }

      ((msgbuf*)msg)->mtype=type;
      memcpy(((msgbuf*)msg)->mtext,buf,len);

      if (msgsnd(q_id,msg,len,0)) {
          free(msg);
          throw IpcError("Unable to send message");
      }
      free(msg);

}

void SysvMsgQueue::Recieve(void* buf,unsigned int len,long type){

    void* msg=malloc(len+sizeof(long));

    if (msg==NULL) {
        throw IpcError("Malloc failed");
    }

    if (msgrcv(q_id,msg,len,type,0)<0) {
        free(msg);
        throw IpcError("Unable to read message");
    }
    memcpy(buf,((msgbuf*)msg)->mtext,len);

    free(msg);

}

void SysvMsgQueue::Remove(void){

	if(msgctl(q_id,IPC_RMID,NULL)){
        throw IpcError("Unable to remove queue");
    }
}

SysvMsgQueue::~SysvMsgQueue() {

}

}
