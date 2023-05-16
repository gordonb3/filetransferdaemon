/*

    libeutils - http://www.excito.com/

    Serial.h - this file is part of libeutils.

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

#ifndef SERIAL_H_
#define SERIAL_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <termios.h>
#include <string>
#include <sys/ioctl.h>
#include <fstream>

#include <ext/stdio_filebuf.h>

namespace EUtils {

class Serial {
public:
	Serial(std::string dev="/dev/ttyS0", speed_t speed=B115200);
	virtual ~Serial();

	void Flush();
	void Drain();
	void SetBaud(speed_t speed);
	int ReadControlSignals();
	void WriteControlSignals(int sig);

	ssize_t Write(const char* buf, ssize_t len);
	ssize_t Read(char* buf, ssize_t len);

	int GetFD(){ return port;}
	std::iostream& GetStream(){return *stream;}

	static speed_t ToSpeed(unsigned int spd);
	static unsigned int SpeedToD(speed_t spd);
private:
	int port;
	__gnu_cxx::stdio_filebuf<char> *fb;
	std::iostream* stream;
	struct termios savetio;
	struct termios newtio;
};

}

#endif /* SERIAL_H_ */
