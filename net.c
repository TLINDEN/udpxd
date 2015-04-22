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
int bindsocket( char* ip, int port ) {
  int fd;
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr( ip );
  addr.sin_port = htons( port );

  fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
  if( -1 == bind( fd, (struct sockaddr*)&addr, sizeof( addr ) ) ) {
    fprintf( stderr, "Cannot bind address (%s:%d)\n", ip, port );
    exit( 1 );
  }
  
  return fd;
}

/* handle new or known incoming requests */
void handle_inside(int inside, char *bindip, struct sockaddr_in *dst) {
  int len;
  unsigned char buffer[MAX_BUFFER_SIZE];
  struct sockaddr_in *src;
  client_t *client;
  int output;
  char *srcip;
  char *dstip = ntoa(dst);
  size_t size = sizeof(struct sockaddr_in);
  src = malloc(size);
  
  len = recvfrom( inside, buffer, sizeof( buffer ), 0, (struct sockaddr*)src, (socklen_t *)&size );
  srcip = ntoa(src);

  if(VERBOSE) {
    char *srcip = ntoa(src);
    fprintf(stderr, "New incomming request from %s:%d with %d bytes\n",
	    srcip, ntohs(src->sin_port), len);
  }

  if(len > 0) {
    /* do we know it ? */
    client = client_find_src(src);
    if(client != NULL) {
      /* yes, we know it, send req out via existing bind socket */
      if(VERBOSE) {
	fprintf(stderr, "Client %s:%d is known, forwarding data to %s:%d\n",
		srcip, ntohs(src->sin_port), dstip, ntohs(dst->sin_port));
  
      }
      if(sendto(client->socket, buffer, len, 0, (struct sockaddr*)dst, size) < 0) {
	fprintf(stderr, "unable to forward to %s:%d\n", dstip, ntohs(dst->sin_port));
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
		srcip, ntohs(src->sin_port), dstip, ntohs(dst->sin_port));
  
      }

      if(bindip == NULL)
	output = bindsocket("0.0.0.0", 0);
      else
	output = bindsocket(bindip, 0);
      
      /* send req out */
      if(sendto(output, buffer, len, 0, (struct sockaddr*)dst, size) < 0) {
	fprintf(stderr, "unable to forward to %s:%d\n", dstip, ntohs(dst->sin_port));
	perror(NULL);
      }
      else {
	struct sockaddr_in *ret = malloc(size);
	getsockname(output, (struct sockaddr*)ret, (socklen_t *)&size);
	client = client_new(output, src, ret);
	client_add(client);
	if(VERBOSE) {
	  if(bindip != NULL) {
	    char *bindip = ntoa(ret);
	    fprintf(stderr, "from %s:%d\n", bindip, ntohs(ret->sin_port));
	  }
	  else {
	    fprintf(stderr, "\n");
	  }
	}
      }
    }
  }

  free(dstip);
  free(srcip);
}

/* handle answer from the outside */
void handle_outside(int inside, int outside) {
  int len;
  unsigned char buffer[MAX_BUFFER_SIZE];
  struct sockaddr_in *src;
  client_t *client;

  size_t size = sizeof(struct sockaddr_in);
  src = malloc(size);
  
  len = recvfrom( outside, buffer, sizeof( buffer ), 0, (struct sockaddr*)src, (socklen_t *)&size );

  if(len > 0) {
    /* do we know it? */
    client = client_find_fd(outside);
    if(client != NULL) {
      /* yes, we know it */
      if(sendto(inside, buffer, len, 0, (struct sockaddr*)client->src, size) < 0) {
	perror("unable to send back to client"); /* FIXME: add src+port */
	client_close(client);
      }
    }
  }
}

