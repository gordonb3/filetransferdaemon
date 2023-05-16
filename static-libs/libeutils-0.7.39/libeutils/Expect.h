/*

    libeutils - http://www.excito.com/

    Expect.h - this file is part of libeutils.

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
#ifndef EXPECT_H_
#define EXPECT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <expect.h>
#include <string.h>

#include <string>
#include <list>
#include <vector>
#include <utility>

namespace EUtils{

class Expect {
public:
	enum Type{
		ExpEnd,
		ExpGlob,
		ExpExact,
		ExpRegexp
	};

	typedef std::pair<Expect::Type,std::string> ExpVal;

	Expect(int echo=0);

	void setTimeout(int time);

	int expect(const std::string& simple);
	int expect(std::vector<Expect::ExpVal>& ev);

	void sendline(const std::string& line);

	std::string match();

	int spawn(int fd);
	int spawn(const std::string& prog,std::list<std::string> args);

	virtual ~Expect();
private:
	int fd;
};

}
#endif /* EXPECT_H_ */
