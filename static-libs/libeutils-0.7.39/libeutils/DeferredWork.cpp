/*

    libeutils - http://www.excito.com/

    DeferredWork.cpp - this file is part of libeutils.

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

#include "DeferredWork.h"

#include <stdlib.h>

namespace EUtils{

typedef struct{
	void (*function)(void*);
	void* payload;

}workunit;

DeferredWork::DeferredWork():AsyncWorker(){
}

DeferredWork& DeferredWork::Instance(){
	static DeferredWork dw;
	return dw;
}

DeferredWork::~DeferredWork(){

}

void DeferredWork::AddWork(void (*function)(void*),void* payload){

	if(!function){
		return;
	}

	workunit* work=(workunit*)malloc(sizeof(workunit));
	if(!work){
		return;
	}
	work->function=function;
	work->payload=payload;
	// printf("AddWork: %p func %p pay %p\n",work,work->function,work->payload);
	this->AddLast((void*)work);

}

void DeferredWork::ProcessElem(void* b){
	// printf("Processing deferred work %p\n",b);
	workunit* work=(workunit*)b;
	// printf("Work %p payload %p\n",work->function,work->payload);
	work->function(work->payload);
}
}
