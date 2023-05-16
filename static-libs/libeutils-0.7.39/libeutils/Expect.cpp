/*

    libeutils - http://www.excito.com/

    Expect.cpp - this file is part of libeutils.

    Copyright (C) 2008 Tor Krill <tor@excito.com>

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
#include "Expect.h"
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>

using namespace std;

namespace EUtils{

Expect::Expect(int echo) {
	this->fd=-1;
	exp_loguser=echo;

}

void Expect::setTimeout(int time){
	exp_timeout=time;
}

Expect::~Expect() {
	// TODO Auto-generated destructor stub
}

int Expect::spawn(int fd){
	this->fd=exp_spawnfd(fd);
	return this->fd;
}

int Expect::spawn(const string& prog, list<string> args){
	int elems=args.size()+1;

	char** argv=(char**)malloc(elems*sizeof(char*));
	if(!argv){
		throw std::runtime_error("Failed to spawn process");
	}

	char* file=strdup(prog.c_str());
	if(!file){
		throw std::runtime_error("Failed to spawn process");
	}

	int i=0;
	for(list<string>::iterator sIt=args.begin();sIt!=args.end();sIt++){
		if(!(argv[i++]=strdup((*sIt).c_str()))){
			throw std::runtime_error("Failed to spawn process");
		}
	}

	argv[i]=NULL;

	if((this->fd=exp_spawnv(file,argv))<0){
		for(i=0;i<elems;i++){
			free(argv[i]);
		}
		free(file);
		free(argv);
		throw std::runtime_error("Failed to spawn process");
	}

	for(i=0;i<elems;i++){
		free(argv[i]);
	}
	free(file);
	free(argv);
	return 0;
}

string Expect::match(){
	ssize_t slen=exp_match_end-exp_match;
	if(slen>0){
		char *buf=(char*)malloc(slen+1);
		if(!buf){
			throw std::runtime_error("Unable to allocate memory");
		}
		memcpy(buf,exp_match,slen);
		buf[slen]='\0';
		string ret(buf);
		free(buf);
		return ret;
	}
	return "";
}

int Expect::expect(const string& simple){
	struct exp_case ec[2];
	ec[0].type=exp_glob;
	ec[0].pattern=strdup(simple.c_str());
	ec[0].value=1;
	ec[1].type=exp_end;
	int retval=exp_expectv(this->fd,ec);
	free(ec[0].pattern);
	return retval;
}
int Expect::expect(vector<Expect::ExpVal>& ev){

	int elems=ev.size()+1;
	struct exp_case *cases=(struct exp_case*)malloc(elems*sizeof(struct exp_case));
	if(!cases){
		throw std::runtime_error("Unable to allocate memory");
	}
	for(unsigned int i=0;i<ev.size();i++){
		switch(ev[i].first){
		case ExpEnd:
			cases[i].type=exp_end;
			break;
		case ExpGlob:
			cases[i].type=exp_glob;
			break;
		case ExpExact:
			cases[i].type=exp_exact;
			break;
		case ExpRegexp:
			cases[i].type=exp_regexp;
			break;
		}
		cases[i].pattern=strdup(ev[i].second.c_str());
		cases[i].value=i+1;
	}
	cases[ev.size()].type=exp_end;

	int retval=exp_expectv(this->fd,cases);
	for(unsigned int i=0;i<ev.size();i++){
		free(cases[i].pattern);
	}
	free(cases);

	return retval;
}
void Expect::sendline(const string& line){
	if(write(this->fd,line.c_str(),line.size())!=(ssize_t)line.size()){
		throw std::runtime_error("Unable to write client");
	}
}

}
