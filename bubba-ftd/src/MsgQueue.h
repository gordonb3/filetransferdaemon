/*
    
    DownloadManager - http://www.excito.com/
    
    MsgQueue.h - this file is part of DownloadManager.
    
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
    
    $Id: MsgQueue.h 1242 2008-04-07 14:07:26Z tor $
*/

#ifndef MY_MSGQUEUE_H
#define MY_MSGQUEUE_H

#include <string>
#include <stdexcept>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>

using namespace std;

class MsgError{
    string errmsg;

public:

    MsgError(string msg=""):errmsg(msg){
    }

    virtual ~MsgError(){
    }

    virtual const string what(){
        return errmsg+": "+string(strerror(errno));
    }
};

class MsgQueue{
private:
    int q_id;
    key_t tok;
public:
    MsgQueue(string path, int token=123);

    void SetGid(gid_t gid);
    void SetUid(uid_t uid);
    void SetPerm(unsigned short mode);

    void Send(const void* buf, unsigned int len, long type=0);

    void Recieve(void* buf, unsigned int len, long msg_type=0);

    virtual ~MsgQueue();
};

#endif
