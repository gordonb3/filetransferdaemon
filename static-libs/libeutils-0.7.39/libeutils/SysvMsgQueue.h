/*

    libeutils - http://www.excito.com/

    SysvMsgQueue.h - this file is part of libeutils.

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

#ifndef SYSVMSGQUEUE_H_
#define SYSVMSGQUEUE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SysvIPC.h"

namespace EUtils {

class SysvMsgQueue: public EUtils::SysvIPC {
private:
    int q_id;
public:
	SysvMsgQueue(const string& path, int token=123);

    void SetGid(gid_t gid);
    void SetUid(uid_t uid);
    void SetPerm(unsigned short mode);

    void Send(const void* buf, unsigned int len, long type=0);

    void Recieve(void* buf, unsigned int len, long msg_type=0);

    void Remove(void);

	virtual ~SysvMsgQueue();
};

}

#endif /* SYSVMSGQUEUE_H_ */
