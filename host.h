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

#ifndef _HAVE_HOST_H
#define _HAVE_HOST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <net/if.h> // if_nametoindex()
#include <ifaddrs.h>

struct _host_t {
  int is_v6;
  struct sockaddr *sock;
  size_t size;
  char *ip;
  int port;
};
typedef struct _host_t host_t;

unsigned get_v6_scope(const char *ip);
int is_linklocal(struct in6_addr *a);
host_t *get_host(char *ip, int port, struct sockaddr_in *v4, struct sockaddr_in6 *v6);
int is_v6(char *ip);
void host_dump(host_t *host);
void host_clean(host_t *host);

#endif
