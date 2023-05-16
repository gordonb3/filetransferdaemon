/*

    libeutils - http://www.excito.com/

    EConfig.h - this file is part of libeutils.

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

    $Id$
*/

#ifndef MY_ECONFIG_H
#define MY_ECONFIG_H

#include <string>
#include <list>
#include <stdexcept>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "Mutex.h"

using namespace std;

namespace EUtils{

class EConfig{
	private:

		/**
		 * Reference to our config file
		 */
		GKeyFile *cfg;

		/**
		 * FD for dnotify
		 */
		 int dnot_fd;

		/**
		 * Stat value of config file
		 */
		struct stat sbuf;

		/**
		 * When was the last stat
		 */
		time_t last_stat;
		/**
		 * where the file changed on last stat?
		 */
		bool stat_positive;

		/**
		 * Is it we who done any writing?
		 * Then we dont need to read back.
		 */
		bool selfwrite;

		/**
		 * Path to file
		 */
		string cfg_path;
		/**
		 * Complete path to config file
		 */
		string cfg_file;

		/* Hide unwanted constructor */
		EConfig(const EConfig& cfg){ throw new std::runtime_error("Illegal copy");}
	    EConfig& operator=(const EConfig&){ throw new std::runtime_error("Illegal assignment");}
	    EConfig& operator=(EConfig&){ throw new std::runtime_error("Illegal assignment");}
	    EConfig& operator=(volatile EConfig&){ throw new std::runtime_error("Illegal assignment");}
	    EConfig& operator=(const volatile EConfig&){ throw new std::runtime_error("Illegal assignment");}
	    EConfig operator=(EConfig){ throw new std::runtime_error("Illegal assignment");}
	protected:
		Mutex file_mutex;
		Mutex cfg_mutex;
		void WriteConfig(void);

		void UpdateFromDisk(bool force=false);

		static int usecount;
		static void SigHandler(int signal);
		static list<EConfig*>* cfglist;
		static Mutex* listlock;
	public:
		EConfig(string cfgpath);

		bool HasGroup(const string& group);
		bool HasKey(const string& group,const string& key);

		string GetValue(const string& group,const string& key);
		void SetValue(const string& group,const string& key,const string& value);

		bool GetBool(const string& group,const string& key);
		int	GetInteger(const string& group,const string& key);

#if (GLIB_CHECK_VERSION(2,12,0))
		double GetDouble(const string& group,const string& key);
		void SetDouble(const string& group,const string& key, const double value);
#endif

		string GetString(const string& group,const string& key);

		void SetBool(const string& group,const string& key, const bool value);
		void SetInteger(const string& group,const string& key, const int value);
		void SetString(const string& group,const string& key, const string& value);

		virtual ~EConfig();
};
}
#endif
