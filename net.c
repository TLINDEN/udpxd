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

#include "net.h"
#include "client.h"
#include "host.h"



char *ntoa(struct sockaddr_in *src) {
  char *ip = malloc(32);
  inet_ntop(AF_INET, (struct in_addr *)&src->sin_addr, ip, 32);
  return ip;
}


/* called each time when the loop restarts to feed select() correctly */
int fill_set(fd_set *fds) {
  int max = 0;

  client_t *current = NULL;
  client_iter(clients, current) {
    if (current->socket < (int)FD_SETSIZE) {
      if (current->socket > max)
	max = current->socket;
      FD_SET(current->socket, fds);
    }
    else {
      fprintf(stderr, "skipped client, socket too large!\n");
    }
  }

  return max;
}

/* return file handle ready to read */
int get_sender(fd_set *fds) {
    int i = 0;

    while(!FD_ISSET(i, fds))
        i++;

    return i;
}


/* bind to a socket, either for listen() or for outgoing src ip binding */
int bindsocket( host_t *sock_h) {
  int fd;
  int err = 0;

  if(sock_h->is_v6) {
    fd = socket( PF_INET6, SOCK_DGRAM, IPPROTO_UDP );
  }
  else {
    fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
  }

  if( -1 == bind( fd, (struct sockaddr*)sock_h->sock, sock_h->size ) ) {
    err = 1;
  }

  if(err) {
    fprintf( stderr, "Cannot bind address ([%s]:%d)\n", sock_h->ip, sock_h->port );
    perror(NULL);
    return -1;
  }
  
  return fd;
}

int start_listener (char *inip, char *inpt, char *srcip, char *dstip, char *dstpt) {
  host_t *listen_h = get_host(inip, atoi(inpt), NULL, NULL);
  host_t *dst_h    = get_host(dstip, atoi(dstpt), NULL, NULL);
  host_t *bind_h   = NULL;

  if(srcip != NULL) {
    bind_h   = get_host(srcip, 0, NULL, NULL);
  }
  else {
    if(dst_h->is_v6)
      bind_h = get_host("::0", 0, NULL, NULL);
    else
      bind_h = get_host("0.0.0.0", 0, NULL, NULL);
  }

  int listen = bindsocket(listen_h);

  if(listen == -1)
    return 1;

  if(VERBOSE) {
    fprintf(stderr, "Listening on %s:%s, forwarding to %s:%s",
	    inip, inpt, dstip, dstpt);
    if(srcip != NULL)
      fprintf(stderr, ", binding to %s\n", srcip);
    else
      fprintf(stderr, "\n");
  }
  
  main_loop(listen, listen_h, bind_h, dst_h);

  return 0;
}

/* handle new or known incoming requests
   FIXME: check client handling:
   http://long.ccaba.upc.es/long/045Guidelines/eva/ipv6.html#daytimeServer6
*/
void handle_inside(int inside, host_t *listen_h, host_t *bind_h, host_t *dst_h) {
  int len;
  unsigned char buffer[MAX_BUFFER_SIZE];
  void *src;
  client_t *client;
  host_t *src_h;
  int output;
  size_t size = listen_h->size;

  src = malloc(size);
  
  len = recvfrom( inside, buffer, sizeof( buffer ), 0,
		  (struct sockaddr*)src, (socklen_t *)&size );

  if(listen_h->is_v6)
    src_h = get_host(NULL, 0, NULL, (struct sockaddr_in6 *)src);
  else
    src_h = get_host(NULL, 0, (struct sockaddr_in *)src, NULL);

  if(VERBOSE) {
    fprintf(stderr, "New incomming request from %s:%d with %d bytes\n",
	    src_h->ip, src_h->port, len);
  }

  if(len > 0) {
    /* do we know it ? */
    client = client_find_src(src_h);
    if(client != NULL) {
      /* yes, we know it, send req out via existing bind socket */
      if(VERBOSE) {
	fprintf(stderr, "Client %s:%d is known, forwarding data to %s:%d\n",
		src_h->ip, src_h->port, dst_h->ip, dst_h->port);
	
      }
      if(sendto(client->socket, buffer, len, 0, (struct sockaddr*)dst_h->sock, dst_h->size) < 0) {
	fprintf(stderr, "unable to forward to %s:%d\n", dst_h->ip, dst_h->port);
	perror(NULL);
      }
      else {
	client_seen(client);
      }
    }
    else {
      /* unknown client, open new out socket */
      if(VERBOSE) {
	fprintf(stderr, "Client %s:%d is unknown, forwarding data to %s:%d ",
		src_h->ip, src_h->port, dst_h->ip, dst_h->port);
  
      }

      output = bindsocket(bind_h);
      
      /* send req out */
      if(sendto(output, buffer, len, 0, (struct sockaddr*)dst_h->sock, dst_h->size) < 0) {
	fprintf(stderr, "unable to forward to %s:%d\n", dst_h->ip, dst_h->port);
	perror(NULL);
      }
      else {
	size = listen_h->size;
	host_t *ret_h;
	if(listen_h->is_v6) {
	  struct sockaddr_in6 *ret = malloc(size);
	  getsockname(output, (struct sockaddr*)ret, (socklen_t *)&size);
	  ret_h = get_host(NULL, 0, NULL, ret);
	  client = client_new(output, src_h, ret_h);
	}
	else {
	  struct sockaddr_in *ret = malloc(size);	  
	  getsockname(output, (struct sockaddr*)ret, (socklen_t *)&size);
	  ret_h = get_host(NULL, 0, ret, NULL);
	  client = client_new(output, src_h, ret_h);
	}

	client_add(client);

	if(VERBOSE) {
	  if(strcmp(bind_h->ip, "0.0.0.0") != 0 || strcmp(bind_h->ip, "::0") != 0) {
	    fprintf(stderr, "from %s:%d\n", ret_h->ip, ret_h->port);
	  }
	  else {
	    fprintf(stderr, "\n");
	  }
	}
      }
    }
  }

  /* FIXME: free? */
}

/* handle answer from the outside */
void handle_outside(int inside, int outside, host_t *outside_h) {
  int len;
  unsigned char buffer[MAX_BUFFER_SIZE];
  void *src;
  client_t *client;

  size_t size = outside_h->size;
  src = malloc(size);
  
  len = recvfrom( outside, buffer, sizeof( buffer ), 0, (struct sockaddr*)src, (socklen_t *)&size );

  if(len > 0) {
    /* do we know it? */
    client = client_find_fd(outside);
    if(client != NULL) {
      /* yes, we know it */
      if(sendto(inside, buffer, len, 0, (struct sockaddr*)client->src, client->size) < 0) {
	perror("unable to send back to client"); /* FIXME: add src+port */
	client_close(client);
      }
    }
  }
}

/* runs forever, handles incoming requests on the inside and answers on the outside */
int main_loop(int listensocket, host_t *listen_h, host_t *bind_h, host_t *dst_h) {
    int max, sender;
    fd_set fds;

    for(;;) {
      FD_ZERO(&fds);
      max = fill_set(&fds);

      FD_SET(listensocket, &fds);
      if (listensocket > max)
	max = listensocket;

      select(max + 1, &fds, NULL, NULL, NULL);

      if (FD_ISSET(listensocket, &fds)) {
	/* incoming client on the inside, get src, bind output fd, add to list
	   if known, otherwise just handle it  */
	handle_inside(listensocket, listen_h, bind_h, dst_h);
      }
      else {
	/* remote answer came in on an output fd, proxy back to the inside */
	sender = get_sender(&fds);
	handle_outside(listensocket, sender, dst_h);
      }

      /* close old outputs, if any */
      client_clean();
    }
}
