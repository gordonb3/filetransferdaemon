/*

    libeutils - http://www.excito.com/

    DeferredWork.h - this file is part of libeutils.

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

    $Id: Condition.h 1119 2007-10-10 22:18:01Z tor $
*/
#ifndef DEFERREDWORK_H_
#define DEFERREDWORK_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AsyncWorker.h"

namespace EUtils{

class DeferredWork:private AsyncWorker{
protected:
	DeferredWork(const DeferredWork&);
	DeferredWork& operator=(const DeferredWork&);

	virtual void ProcessElem(void* b);
public:
	DeferredWork();
	static DeferredWork& Instance();
	void AddWork(void (*function)(void*),void* payload);

	virtual ~DeferredWork();
};
}
#endif /*DEFERREDWORK_H_*/
