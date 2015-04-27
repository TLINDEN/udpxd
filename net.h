/*
    This file is part of udpxd.

    Copyright (C) 2015 T.v.Dein.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    You can contact me by mail: <tom AT vondein DOT org>.
*/

#ifndef _HAVE_NET_H
#define _HAVE_NET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <syslog.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <pwd.h>


#include "client.h"

#define MAX_BUFFER_SIZE 65535

extern client_t *clients;
extern int VERBOSE;
extern int FORKED;



void handle_inside(int inside, host_t *listen_h, host_t *bind_h, host_t *dst_h);
void handle_outside(int inside, int outside, host_t *outside_h);

int main_loop(int listensocket, host_t *listen_h, host_t *bind_h, host_t *dst_h);
int start_listener (char *inip, char *inpt, char *srcip, char *dstip,
		    char *dstpt, char *pidfile, char *chrootdir, char *user);
int daemonize(char *pidfile);
int drop_privileges(char *user, char *chrootdir);

int fill_set(fd_set *fds);
int get_sender(fd_set *fds);
int bindsocket( host_t *sock_h);
void int_handler(int  sig);
void verb_prbind (host_t *bind_h);

#define _IS_LINK_LOCAL(a) do { IN6_IS_ADDR_LINKLOCAL(a); } while(0)

#endif
