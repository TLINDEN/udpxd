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

#ifndef _HAVE_CLIENT_H
#define _HAVE_CLIENT_H

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


#ifndef byte
typedef uint8_t byte;
#endif

#include "uthash.h"

#define MAXAGE         3600 /* seconds after which to close outgoing sockets and forget client src */

struct _client_t {
  int socket;               /* bind socket for outgoing traffic */
  struct sockaddr_in *src;  /* client src (ip+port) from incoming socket */
  struct sockaddr_in *dst;  /* client dst (ip+port) to outgoing socket */
  uint64_t lastseen;        /* when did we recv last time from it */
  UT_hash_handle hh;
};
typedef struct _client_t client_t;

extern client_t *clients;
extern int VERBOSE;

/*  wrapper for HASH_ITER */
/** Iterate over the list of clients.

    Sample use:

    @code
    client_t *current = NULL;
    client_iter(clientlist, current) {
      dosomething(current)
    }
    @endcode
*/
#define client_iter(clients, client)               \
  client_t *__c = NULL;				 \
  HASH_ITER(hh, clients, client, __c)


void client_del(client_t *client);
void client_add(client_t *client);
void client_seen(client_t *client);
void client_close(client_t *client);
void client_clean();

client_t *client_find_fd(int fd);
client_t *client_find_src(struct sockaddr_in *src);
client_t *client_new(int fd, struct sockaddr_in *src, struct sockaddr_in *dst);


#endif
