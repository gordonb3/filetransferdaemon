/*
 * =====================================================================================
 *
 *       Filename:  Process.cpp
 *
 *    Description:  Handles simple calls to external processes
 *
 *        Version:  1.0
 *        Created:  04/07/2009 04:38:30 PM CEST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Carl Fürstenberg (carl@excito.com),
 *        Company:  Excito
 *
 * =====================================================================================
 */

#include "Process.h"
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <algorithm>
#include <iostream>

#define BUFFER_SIZE 1024

using namespace EUtils;
using namespace std;

int Process::call(const char** const cmd) {
	return call(const_cast<char** const>(cmd));
}
int Process::call(char** const cmd) {

	char buffer[BUFFER_SIZE];
	int len;
	for (int i = 0; i < 3; i++) {
		if (pipe(this->pipes[i]) == -1) {
			perror("pipe failed");
			exit(EXIT_FAILURE);
		}
	}

	// Handle STDIN for the called program
	// We do this early to make sure stdin is filled and closed on this side
	// before we fork.
	// Though this means that we cant allow further filling of stdin after we have
	// begun calling.
	while(len = pin->rdbuf()->sgetn( buffer, BUFFER_SIZE )) {
		if( write( pipes[0][1], buffer, len ) == -1 ) {
			perror("werite failed");
			exit(EXIT_FAILURE);
		}
		memset( buffer, 0, BUFFER_SIZE );
	}
	close(pipes[0][1]);

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork failed");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) { /* child */

		close( pipes[1][0] );
		close( pipes[2][0] );

		dup2( pipes[0][0], 0 );
		dup2( pipes[1][1], 1 );
		dup2( pipes[2][1], 2 );


		int retval = execvp(cmd[0], cmd);
		if (retval == -1) {
			perror("execvp failed");
			exit(EXIT_FAILURE);
		}
		exit( retval );

	} else { /* parent */

		int status, selectval;

		close( pipes[0][0] );
		close( pipes[1][1] );
		close( pipes[2][1] );

		int rfd = pipes[1][0];
		int efd = pipes[2][0];

		bool done = false;

		while(!done) {
			if( waitpid( pid, &status, WNOHANG ) != 0 ) {
				if(WIFEXITED(status))
					done = true;
			}
			fd_set fds;
			int nfds;

			FD_ZERO(&fds);

			FD_SET( rfd, &fds );
			nfds = rfd;

			FD_SET( efd, &fds );
			nfds = max( nfds, efd );

			selectval = select( nfds + 1,  &fds,  NULL, NULL, 0 );
			if (selectval == -1 && errno == EINTR)
				continue;
			if (selectval < 0) {
				perror("select()");
				exit(EXIT_FAILURE);
			}

			memset( buffer, 0, BUFFER_SIZE );
			if( FD_ISSET( rfd, &fds ) ) {
				len = read( rfd, &buffer, BUFFER_SIZE );
				if( len > 0 ) {
					pout->rdbuf()->sputn( buffer, len );
				}
			}
			memset( buffer, 0, BUFFER_SIZE );
			if( FD_ISSET( efd, &fds ) ) {
				len = read( efd, buffer, BUFFER_SIZE );
				if( len > 0 ) {
					perr->rdbuf()->sputn( buffer, len );
				}
			}
			fflush(NULL);

		}
		close( rfd );
		close( efd );
		return WEXITSTATUS(status);
	}
}

