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

#include "udpxd.h"
#include "net.h"
#include "client.h"

/* global client list */
client_t *clients = NULL;
int VERBOSE = 0;
int FORKED = 0;

/* parse ip:port */
int parse_ip(char *src, char *ip, char *pt) {
  char *ptr = NULL;

  if (strchr(optarg, '[')) {
    /* v6 */
    ptr = strtok(&src[1], "]");

    if(strlen(ptr) > INET6_ADDRSTRLEN) {
      fprintf(stderr, "ip v6 address is too long!\n");
      return 1;
    }
    
    strncpy(ip, ptr, strlen(ptr)+1);
    ptr = strtok(NULL, "]");
    if(ptr)
      ptr = &ptr[1]; /* remove : */
  }
  else if(strchr(optarg, ':')) {
    /* v4 */
    ptr = strtok(src, ":");
    
    if(strlen(ptr) > INET_ADDRSTRLEN) {
      fprintf(stderr, "ip v4 address is too long!\n");
      return 1;
    }
    
    strncpy(ip, ptr, strlen(ptr)+1);
    ptr = strtok(NULL, ":");
  }
  else {
    fprintf(stderr, "Invalid ip/port specification!\n");
    return 1;
  }

  if(ptr != NULL) {
    /* got a port */
    if(strlen(ptr) > 5) {
      fprintf(stderr, "port is too long!\n");
      return 1;
    }
    else {
      if(atoi(ptr) > 65535) {
        fprintf(stderr, "maximum port number possible: 65535!\n");
        return 1;
      }
      strncpy(pt, ptr, strlen(ptr)+1);
    }
  }
  else {
    fprintf(stderr, "Port is missing!\n");
    return 1;
  }

  return 0;
}



void usage() {
  fprintf(stderr,
          "Usage: udpxd [-lbdfpvhV]\n\n"
          "Options:\n"
          "--listen     -l <ip:port>     listen for incoming requests\n"
          "--bind       -b <ip>          bind ip used for outgoing requests\n"
          "--to         -t <ip:port>     destination to forward requests to\n"
          "--daemon     -d               daemon mode, fork into background\n"
          "--pidfile    -p <file>        pidfile, default: /var/run/udpxd.pid\n"
          "--user       -u <user>        run as user (only in daemon mode)\n"
          "--chroot     -c <path>        chroot to <path> (only in daemon mode)\n"
          "--help       -h -?            print help message\n"
          "--version    -V               print program version\n"
          "--verbose    -v               enable verbose logging\n\n"
          "Options -l and -t are mandatory.\n\n"
          "This is udpxd version %s.\n", UDPXD_VERSION
          );
}



int main ( int argc, char* argv[] ) {
  int opt, err;
  char *inip, *inpt, *srcip, *dstip, *dstpt;
  char pidfile[MAX_BUFFER_SIZE];
  char user[128];
  char chroot[MAX_BUFFER_SIZE];

  err = 0;
  
  static struct option longopts[] = {
    { "listen",    required_argument, NULL,           'l' },
    { "bind",      required_argument, NULL,           'b' },
    { "to",        required_argument, NULL,           't' },
    { "version",   no_argument,       NULL,           'V' },
    { "help",      no_argument,       NULL,           'h' },
    { "verbose",   no_argument,       NULL,           'v' },
    { "daemon",    no_argument,       NULL,           'd' },
    { "pidfile",   required_argument, NULL,           'p' },
    { "user",      required_argument, NULL,           'u' },
    { "chroot",    required_argument, NULL,           'c' },
  };

  if( argc < 2 ) {
    usage();
    return 1;
  }

  srcip = dstip = inip = dstpt = inpt = NULL;

  /* set defaults */
  strncpy(pidfile, "/var/run/udpxd.pid", 19);
  strncpy(user, "nobody", 7);
  strncpy(chroot, "/var/empty", 11);
  
  while ((opt = getopt_long(argc, argv, "l:b:t:u:c:vdVh?", longopts, NULL)) != -1) {
    switch (opt) {
    case 'V':
      fprintf(stderr, "This is %s version %s\n", argv[0], UDPXD_VERSION);
      return 1;
      break;
    case 'd':
      FORKED = 1;
      break;
    case 'h':
    case '?':
      usage();
      return 1;
      break;
    case 'v':
      VERBOSE = 1;
      break;
    case 'l':
      inip  = malloc(INET6_ADDRSTRLEN+1);
      inpt  = malloc(6);
      if (parse_ip(optarg, inip, inpt) != 0) {
        fprintf(stderr, "Parameter -l has the format <ip-address:port>!\n");
        err = 1;
      }
      break;
    case 't':
      dstip = malloc(INET6_ADDRSTRLEN+1);
      dstpt = malloc(6);
      if (parse_ip(optarg, dstip, dstpt) != 0) {
        fprintf(stderr, "Parameter -d has the format <ip-address:port>!\n");
        err = 1;
      }
      break;
    case 'b':
      if(strlen(optarg) > INET6_ADDRSTRLEN) {
        fprintf(stderr, "Bind ip address is too long!\n");
        err = 1;
      }
      srcip = malloc(INET6_ADDRSTRLEN+1);
      strncpy(srcip, optarg, strlen(optarg));
      break;
    case 'p':
      strncpy(pidfile, optarg, strlen(optarg));
      break;
    case 'u':
      strncpy(user, optarg, strlen(optarg));
      break;
    case 'c':
      strncpy(chroot, optarg, strlen(optarg));
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
    err = 1;
  }
  
  if(dstip == NULL) {
    fprintf(stderr, "-t parameter is required!\n");
    usage();
    err = 1;
  }

  if(srcip != NULL && dstip != NULL) {
    if(is_v6(srcip) != is_v6(dstip)) {
      fprintf(stderr, "Bind ip and destination ip must be both v4 or v6 and can't be mixed!\n");
      err = 1;
    }
  }

  if(! err) {
    err = start_listener (inip, inpt, srcip, dstip, dstpt, pidfile, chroot, user);
  }
  
  if(srcip != NULL)
    free(srcip);
  if(dstip != NULL)
    free(dstip);
  if(inip != NULL)
    free(inip);
  if(inpt != NULL)
    free(inpt);
  if(dstpt != NULL)
    free(dstpt);

  return err;
}
