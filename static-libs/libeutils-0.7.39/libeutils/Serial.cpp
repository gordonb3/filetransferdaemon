/*

    libeutils - http://www.excito.com/

    Serial.cpp - this file is part of libeutils.

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
#include "Serial.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>

#include <stdexcept>

namespace EUtils {

using namespace std;

Serial::Serial(string dev, speed_t speed) {
	port=open(dev.c_str() ,O_RDWR|O_NOCTTY,O_SYNC);

	if (port<0) {
		throw std::runtime_error("Couldn't open: "+dev);
	}

	if ( tcgetattr(port,&savetio)<0) {
		close(port);
		throw std::runtime_error("Couldn't save termios");
	}

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = speed | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 1;

	tcflush(port, TCIFLUSH);
	if (tcsetattr(port, TCSANOW, &newtio)<0) {
		close(port);
		throw std::runtime_error("Couldn't init new termio");
    }
    this->fb=new __gnu_cxx::stdio_filebuf<char>(port,std::ios_base::out|std::ios_base::in);
	this->stream=new iostream(fb);
}

ssize_t Serial::Read(char* buf, ssize_t len){
	ssize_t r;
	if ( (r=read(port,buf,len))<0) {
		throw std::runtime_error("Couldn't read block");
	}

	return r;

}

ssize_t Serial::Write(const char* buf, ssize_t len){
	ssize_t r;
	if ( (r=write(port,buf,len))<0) {
		throw std::runtime_error("Couldn't write block");
	}

	return r;
}

Serial::~Serial() {
	tcsetattr(port, TCSANOW, &savetio);
	close(port);

}

void Serial::SetBaud(speed_t speed){
    tcdrain(port);

	cfsetospeed(&newtio, speed);
	cfsetispeed(&newtio, speed);

	tcsetattr(port, TCSANOW, &newtio);
}

void Serial::Flush(){
	tcflush(port, TCIOFLUSH);
}

void Serial::Drain(){
	tcdrain(port);
}

int Serial::ReadControlSignals(){
    int status,ret;

	ret = ioctl(port, TIOCMGET, &status);
	if (ret < 0) {
		throw std::runtime_error("Couldn't read controlsignals");
	}

	return status;
}

void Serial::WriteControlSignals(int sig){
    int ret;

	ret = ioctl(port, TIOCMSET,&sig);
	if (ret < 0) {
		throw std::runtime_error("Couldn't set controlsignals");
	}
}

speed_t Serial::ToSpeed(unsigned int spd) {
	switch (spd) {
	case 50:
		return B50;
		break;
	case 75:
		return B75;
		break;
	case 110:
		return B110;
		break;
	case 134:
		return B134;
		break;
	case 150:
		return B150;
		break;
	case 200:
		return B200;
		break;
	case 300:
		return B300;
		break;
	case 600:
		return B600;
		break;
	case 1200:
		return B1200;
		break;
	case 1800:
		return B1800;
		break;
	case 2400:
		return B2400;
		break;
	case 4800:
		return B4800;
		break;
	case 9600:
		return B9600;
		break;
	case 19200:
		return B19200;
		break;
	case 38400:
		return B38400;
		break;
	case 57600:
		return B57600;
		break;
	case 115200:
		return B115200;
		break;
	case 230400:
		return B230400;
		break;
	case 460800:
		return B460800;
		break;
	case 500000:
		return B500000;
		break;
	case 576000:
		return B576000;
		break;
	case 921600:
		return B921600;
		break;
	case 1000000:
		return B1000000;
		break;
	case 1152000:
		return B1152000;
		break;
	default:
		throw std::runtime_error("Illegal speed");
		return B0;
	}
}

unsigned int Serial::SpeedToD(speed_t spd) {
	switch (spd) {
	case B50:
		return 50;
		break;
	case B75:
		return 75;
		break;
	case B110:
		return 110;
		break;
	case B134:
		return 134;
		break;
	case B150:
		return 150;
		break;
	case B200:
		return 200;
		break;
	case B300:
		return 300;
		break;
	case B600:
		return 600;
		break;
	case B1200:
		return 1200;
		break;
	case B1800:
		return 1800;
		break;
	case B2400:
		return 2400;
		break;
	case B4800:
		return 4800;
		break;
	case B9600:
		return 9600;
		break;
	case B19200:
		return 19200;
		break;
	case B38400:
		return 38400;
		break;
	case B57600:
		return 57600;
		break;
	case B115200:
		return 115200;
		break;
	case B230400:
		return 230400;
		break;
	case B460800:
		return 460800;
		break;
	case B500000:
		return 500000;
		break;
	case B576000:
		return 576000;
		break;
	case B921600:
		return 921600;
		break;
	case B1000000:
		return 1000000;
		break;
	case B1152000:
		return 1152000;
		break;
	default:
		throw std::runtime_error("Illegal speed");
		return 0;
	}
}


}
