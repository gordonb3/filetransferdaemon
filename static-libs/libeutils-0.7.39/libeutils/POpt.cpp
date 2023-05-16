/*

    libeutils - http://www.excito.com/

    POpt.cpp - this file is part of libeutils.

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


#include "POpt.h"
#include <stdlib.h>

using namespace std;

namespace EUtils{

static const char* helpstr="Help options:";

POpt::POpt():parse_options(NULL){

}

POpt::POpt(list<Option> opts): parse_options(NULL){
	for(list<Option>::iterator oIt=opts.begin();oIt!=opts.end();oIt++){
		this->options[(*oIt).longName]=*oIt;
	}

}


void POpt::parsecallback(poptContext con, enum poptCallbackReason reason,
								const struct poptOption * opt, const char * arg,
								void * data){

	if(!opt){
		return;
	}

	if(reason != POPT_CALLBACK_REASON_OPTION){
		return;
	}

	POpt* popt=static_cast<POpt*>(data);

	switch(opt->argInfo){
	case POPT_ARG_NONE:
		popt->options[opt->longName].value="true";
		break;
	case POPT_ARG_STRING:
	case POPT_ARG_INT:
	case POPT_ARG_LONG:
#ifdef POPT_ARG_LONGLONG
		/* Added in popt 1.14 */
	case POPT_ARG_LONGLONG:
#endif
	case POPT_ARG_VAL:
	case POPT_ARG_FLOAT:
	case POPT_ARG_DOUBLE:
		popt->options[opt->longName].value=arg;
		break;
	default:
		//ERROR
		break;
	}

}

void POpt::AddOption(const Option& opt){
	this->options[opt.longName]=opt;
}

static void makeopt(
		struct poptOption* opt,const char* name=NULL,char sname=0, int arginfo=0,
		void* arg=NULL,int val=0, const char* desc=NULL, const char* adesc=NULL)
{

	opt->longName=name;
	opt->shortName=sname;
	opt->argInfo=arginfo;
	opt->arg=arg;
	opt->val=val;
	opt->descrip=desc;
	opt->argDescrip=adesc;
}

void POpt::buildparseopts(){
	int tot_opts=this->options.size()+3;
	this->parse_options=(poptOption*)malloc( tot_opts*sizeof(struct poptOption));
	if(!this->parse_options){
		// err
		return;
	}
	int i=1;
	map<string,Option>::iterator oIt=this->options.begin();
	for(;oIt!=this->options.end();oIt++,i++){
		makeopt(&this->parse_options[i],
			(*oIt).second.longName.c_str(),
			(*oIt).second.shortName,
			(*oIt).second.type,
			NULL,
			0,
			(*oIt).second.desc.c_str(),
			(*oIt).second.argDesc.c_str()
		);
	}
	makeopt(&this->parse_options[0],NULL,0,POPT_ARG_CALLBACK,(void*)&POpt::parsecallback,0,(const char*)this);
	makeopt(&this->parse_options[tot_opts-2],NULL,0,POPT_ARG_INCLUDE_TABLE,poptHelpOptions,0,helpstr,NULL);
	makeopt(&this->parse_options[tot_opts-1]);

}

void POpt::cleanparseopts(){
	if(this->parse_options){
		free(this->parse_options);
		this->parse_options=NULL;
	}
}

bool POpt::Parse(int argc, char** argv){

	// TODO: Error or not?
	if(argc<2){
		return true;
	}

	this->remainder.clear();
	this->buildparseopts();

	poptContext optCon = poptGetContext(NULL,argc,(const char**)argv,this->parse_options,0);

	int rc;
	if((rc=poptGetNextOpt(optCon))<-1){
		poptFreeContext(optCon);
		this->cleanparseopts();
		return false;
	}

	const char *arg;
	while((arg=poptGetArg(optCon))){
		this->remainder.push_back(arg);
	}

	poptFreeContext(optCon);
	this->cleanparseopts();
	return true;
}

string& POpt::GetValue(const char* longname){
	return this->options[longname].value;
}

string& POpt::operator[](const char* longname){
	return this->GetValue(longname);
}

Option& POpt::GetOption(const char* longname){
	return this->options[longname];
}

list<string> POpt::Remainder(){
	return this->remainder;
}

POpt::~POpt(){
	this->cleanparseopts();
}

}
