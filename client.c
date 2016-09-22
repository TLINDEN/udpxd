/*
    This file is part of udpxd.

    Copyright (C) 2015-2016 T.v.Dein.

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

#include "client.h"
#include "log.h"

void client_del(client_t *client) {
  HASH_DEL(clients, client);
}

void client_add(client_t *client) {
  HASH_ADD_INT(clients, socket, client);
}

client_t *client_find_fd(int fd) {
  client_t *client = NULL;
  HASH_FIND_INT(clients, &fd, client);
  return client; /*  maybe NULL! */
}

client_t *client_find_src(host_t *src) {
  client_t *current = NULL;
  client_iter(clients, current) {
    if(strcmp(current->src->ip, src->ip) == 0 && current->src->port == src->port)
      return current;
  }
  return NULL;
}

void client_seen(client_t *client) {
  client->lastseen = (long)time(0);
}

client_t *client_new(int fd, host_t *src, host_t *dst) {
  client_t *client = malloc(sizeof(client_t));
  client->socket = fd;
  client->src = src;
  client->dst = dst;
  client_seen(client);
  return client;
}

void client_close(client_t *client) {
  client_del(client);
  close(client->socket);
  host_clean(client->src);
  host_clean(client->dst);
  free(client);
}

void client_clean(int asap) {
  uint32_t now = (long)time(0);
  uint32_t diff;
  client_t *current;
  client_iter(clients, current) {
    diff = now - current->lastseen;
    if(diff >= MAXAGE || asap) {
      verbose("closing socket %s:%d for client %s:%d (aged out after %d seconds)\n",
              current->src->ip, current->src->port, current->dst->ip, current->dst->port, MAXAGE);
      client_close(current);
    }
  }
}

