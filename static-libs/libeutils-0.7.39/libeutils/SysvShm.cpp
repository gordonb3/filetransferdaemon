/*

    libeutils - http://www.excito.com/

    SysvShm.cpp - this file is part of libeutils.

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

#include "SysvShm.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace EUtils {

SysvShm::SysvShm(size_t size, const string& path, int token):SysvIPC(path,token) {
	/* connect to (and possibly create) the segment: */
	if ((this->shmid = shmget(this->tok, size, 0644 | IPC_CREAT)) == -1) {
		throw IpcError("Failed to get shm");
	}

	this->data = shmat(this->shmid, (void *)0, 0);
	if (this->data == (char *)(-1)) {
		throw IpcError("Failed to attach to shm");
	}

}

void* SysvShm::operator *(){
	return this->data;
}

void* SysvShm::Value(){
	return this->operator *();
}

void SysvShm::Remove(){
	if(shmctl(this->shmid,IPC_RMID,NULL)==-1){
		throw IpcError("Failed to remove shm");
	}
}

SysvShm::~SysvShm() noexcept(false) {
	if (shmdt(data) == -1) {
		throw IpcError("Failed to detach shm");
	}
}

}
