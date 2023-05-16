/*
 * =====================================================================================
 *
 *       Filename:  Process.h
 *
 *    Description:  Handles simple calls to external processes
 *
 *        Version:  1.0
 *        Created:  04/07/2009 04:38:33 PM CEST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Carl Fürstenberg (carl@excito.com),
 *        Company:  Excito
 *
 * =====================================================================================
 */

#ifndef PROCESS_H
#define PROCESS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <cstdarg>

#include <string>
#include <sstream>
#include <memory>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace EUtils {
	class Process {
	public:
		Process() :
			pin(new std::ostringstream( std::ios_base::in )),
			pout(new std::istringstream( std::ios_base::out )),
			perr(new std::istringstream( std::ios_base::out ))
		{ }

		~Process() { }
		/**
		 * \brief Calls an external program
		 *
		 * \param cmd the commando to be called. the array must be ended with an NULL pointer
		 *
		 * \return exit status from executed program
		 *
		 * Example:
		 * \code
		 * const char* cmd[] = { "echo", "foo", NULL };
		 * p.call(cmd);
		 * std::cout << p;
		 * \endcode
		 */
		int call(char** const cmd);
		int call(const char** const cmd);

		std::unique_ptr<std::ostringstream> pin;
		std::unique_ptr<std::istringstream> pout;
		std::unique_ptr<std::istringstream> perr;


	private:
		int pipes[3][2];
	};

}

#endif //PROCESS_H
