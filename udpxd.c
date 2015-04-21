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

#include "udpxd.h"
#include "net.h"
#include "client.h"

/* global client list */
client_t *clients = NULL;
int VERBOSE = 0;

/* runs forever, handles incoming requests on the inside and answers on the outside */
int main_loop(int listensocket, char *bindip, struct sockaddr_in *dst) {
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
	handle_inside(listensocket, bindip, dst);
      }
      else {
	/* remote answer came in on an output fd, proxy back to the inside */
	sender = get_sender(&fds);
	handle_outside(listensocket, sender);
      }

      /* close old outputs, if any */
      client_clean();
    }
}


int main ( int argc, char* argv[] ) {
  int listen, opt;
  char *inip, *inpt, *srcip, *dstip, *dstpt;
  struct sockaddr_in *dst;
  char colon[] = ":";
  
  static struct option longopts[] = {
    { "listen",    required_argument, NULL,           'l' },
    { "bind",      required_argument, NULL,           'b' },
    { "dest",      required_argument, NULL,           'd' },
    { "version",   no_argument,       NULL,           'v' },
    { "help",      no_argument,       NULL,           'h' },
    { "verbose",   no_argument,       NULL,           'V' }
  };

  if( argc < 2 ) {
    usage();
    return 1;
  }

  srcip = dstip = inip = dstpt = inpt = NULL;
    
  while ((opt = getopt_long(argc, argv, "l:b:d:vVh?", longopts, NULL)) != -1) {
    switch (opt) {
    case 'v':
      fprintf(stderr, "This is %s version %s\n", argv[0], UDPXD_VERSION);
      return 1;
      break;
    case 'h':
    case '?':
      usage();
      return 1;
      break;
    case 'V':
      VERBOSE = 1;
      break;
    case 'l':
      if(strchr(optarg, ':')) {
	  char *ptr = NULL;
	  ptr = strtok(optarg, colon);
	  inip = malloc( strlen(ptr)+1);
	  strncpy(inip, ptr, strlen(ptr)+1);
	  ptr = strtok(NULL, colon);
	  if(ptr != NULL) {
	    inpt = malloc( strlen(ptr)+1);
	    strncpy(inpt, ptr, strlen(ptr)+1);
	  }
	  else {
	    fprintf(stderr, "Listen port for parameter -l is missing!\n");
	    return 0;
	  }
      }
      else {
	fprintf(stderr, "Parameter -l has the format <ip-address:port>!\n");
	return 0;
      }
      break;
    case 'd':
      if(strchr(optarg, ':')) {
	  char *ptr = NULL;
	  ptr = strtok(optarg, colon);
	  dstip = malloc( strlen(ptr)+1);
	  strncpy(dstip, ptr, strlen(ptr)+1);
	  ptr = strtok(NULL, colon);
	  if(ptr != NULL) {
	    dstpt = malloc( strlen(ptr)+1);
	    strncpy(dstpt, ptr, strlen(ptr)+1);
	  }
	  else {
	    fprintf(stderr, "Destination port for parameter -d is missing!\n");
	    return 0;
	  }
      }
      else {
	fprintf(stderr, "Parameter -d has the format <ip-address:port>!\n");
	return 0;
      }
      break;
    case 'b':
      srcip = malloc(strlen(optarg));
      strncpy(srcip, optarg, strlen(optarg));
      break;
    default:
      usage();
      return 1;
      break;
    }
  }

  if(inip == NULL) {
    fprintf(stderr, "-l parameter is required!\n");
    usage();
    return 1;
  }
  
  if(dstip == NULL) {
    fprintf(stderr, "-d parameter is required!\n");
    usage();
    return 1;
  }

  listen = bindsocket(inip, atoi(inpt));

  dst = malloc(sizeof(struct sockaddr_in));
  dst->sin_family = AF_INET;
  dst->sin_addr.s_addr = inet_addr( dstip );
  dst->sin_port = htons( atoi( dstpt ) );

  if(VERBOSE) {
    fprintf(stderr, "Listening on %s:%s, forwarding to %s:%s",
	    inip, inpt, dstip, dstpt);
    if(srcip != NULL)
      fprintf(stderr, ", binding to %s\n", srcip);
    else
      fprintf(stderr, "\n");
  }
  
  main_loop(listen, srcip, dst);

  /* FIXME: add sighandler, clean up mem */
  
  return 0;
}

void usage() {
  fprintf(stderr,
	  "Usage: udpxd [-lbdvhV]\n\n"
	  "Options:\n"
	  "--listen  -l <ip:port>     listen for incoming requests\n"
	  "--bind    -b <ip>          bind ip used for outgoing requests\n"
	  "--dest    -d <ip:port>     destination to forward requests to\n"
	  "--help    -h -?            print help message\n"
	  "--version -v               print program version\n"
	  "--verbose -V               enable verbose logging\n\n"
	  "Options -l and -d are mandatory.\n\n"
	  "This is udpxd version %s.\n", UDPXD_VERSION
	  );
}
