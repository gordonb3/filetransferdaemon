/*

    libeutils - http://www.excito.com/

    Regex.cpp - this file is part of libeutils.

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

#include "Regex.h"
#include <stdexcept>
#include <stdlib.h>
#include <iostream>

namespace EUtils{

Regex::Regex(const string& regex, bool caseinsens){
	this->Compile(regex,caseinsens);
}

string Regex::GetError(int err){
	char buf[256];
	regerror(err, &this->preg,buf,256);
	return string(buf);
}


void Regex::Compile(const string& regex, bool caseinsens){
	int flags=REG_EXTENDED;
	int retval;
	if(caseinsens){
		flags|=REG_ICASE;
	}
	if((retval=regcomp(&this->preg,regex.c_str(),flags))!=0){
		this->regvalid=false;
		throw new std::runtime_error(this->GetError(retval));
	}else{
		this->regvalid=true;
	}
}

unsigned int Regex::NextMatch(Regex::Matches& match){
	if(!this->regvalid){
		throw new std::runtime_error("No valid regexp");
	}

	Matches ret;

	// Make sure return value empty
	match.clear();

	regmatch_t *matches;
	matches=(regmatch_t *)calloc(sizeof(regmatch_t),this->maxmatch);
	if(!matches){
		throw new std::runtime_error("Failed to allocate memory");
	}
	if(regexec(&this->preg,
		this->curline.substr(this->curpos,string::npos).c_str(),
		this->maxmatch,matches,0)==0){

		for(int i=0;i<maxmatch;i++){
			if(matches[i].rm_so!=-1){
				ret.push_back(pair<int,int>((this->curpos)+matches[i].rm_so,(this->curpos)+matches[i].rm_eo));
			}
		}

		this->curpos+=matches[0].rm_eo;

		// Copy to returnvalue
		match.insert(match.begin(),ret.begin()+1,ret.end());
	}

	free(matches);

	return match.size();
}

bool Regex::Match(const string& val, int maxmatch){

	if(!this->regvalid){
		throw new std::runtime_error("No valid regexp");
	}

	this->maxmatch=maxmatch;
	this->curline=val;
	this->curpos=0;

	//TODO: Cache first match
	if(regexec(&this->preg,val.c_str(),0,0,0)==0){
		return true;
	}

	return false;
}

void Regex::FreeRegex(){
	regfree(&this->preg);
	this->regvalid=false;
}

Regex::~Regex(){
	if(this->regvalid){
		this->FreeRegex();
	}
}

};
