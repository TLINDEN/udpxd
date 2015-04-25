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

#include "host.h"

/* fill a generic struct depending of what we already know,
   which is easier to pass between functions,
   maybe v4 or v6, filled from existing structs or from strings,
   which create the sockaddr* structs */
host_t *get_host(char *ip, int port, struct sockaddr_in *v4, struct sockaddr_in6 *v6) {
  host_t *host = malloc(sizeof(host_t));
  host->sock = NULL;
  host->is_v6 = 0;
  host->port = port;

  if(ip != NULL) {
    if(is_v6(ip)) {
      struct sockaddr_in6 *tmp = malloc(sizeof(struct sockaddr_in6));
      memset(tmp, 0, sizeof(struct sockaddr_in6));
      
      inet_pton(AF_INET6, ip, (struct in6_addr*)&tmp->sin6_addr);
      
      tmp->sin6_family = AF_INET6;
      tmp->sin6_port = htons( port );
 
      if (is_linklocal((struct in6_addr*)&tmp->sin6_addr))
	tmp->sin6_scope_id = get_v6_scope(ip);
      else
	tmp->sin6_scope_id = 0;

      host->is_v6 = 1;
      host->sock  = (struct sockaddr*)tmp;
      host->size = sizeof(struct sockaddr_in6);
      host->ip = malloc(INET6_ADDRSTRLEN+1);
      memcpy(host->ip, ip, INET6_ADDRSTRLEN);
    }
    else {
      struct sockaddr_in *tmp = malloc(sizeof(struct sockaddr_in));
      memset(tmp, 0, sizeof(struct sockaddr_in));
      tmp->sin_family = AF_INET;
      tmp->sin_addr.s_addr = inet_addr( ip );
      tmp->sin_port = htons( port );
      host->sock   = (struct sockaddr*)tmp;
      host->size = sizeof(struct sockaddr_in);
      host->ip = malloc(INET_ADDRSTRLEN+1);
      memcpy(host->ip, ip, INET_ADDRSTRLEN);
    }
  }
  else if(v4 != NULL) {
    struct sockaddr_in *tmp = malloc(sizeof(struct sockaddr_in));
    memcpy(tmp, v4, sizeof(struct sockaddr_in));
    host->ip = malloc(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, (struct in_addr *)&tmp->sin_addr, host->ip, INET_ADDRSTRLEN);
    host->port = ntohs(tmp->sin_port);
    host->sock = (struct sockaddr*)tmp;
    host->size = sizeof(struct sockaddr_in);
    //fprintf(stderr, "%s sock: %p\n", host->ip, tmp);
  }
  else if(v6 != NULL) {
    struct sockaddr_in6 *tmp = malloc(sizeof(struct sockaddr_in6));
    memcpy(tmp, v6, sizeof(struct sockaddr_in6));
    host->ip = malloc(INET6_ADDRSTRLEN);
    inet_ntop(AF_INET, (struct in6_addr *)&tmp->sin6_addr, host->ip, INET6_ADDRSTRLEN);
    host->port = ntohs(tmp->sin6_port);
    host->sock = (struct sockaddr*)tmp;
    host->is_v6 = 1;
    host->size = sizeof(struct sockaddr_in6);
  }
  else {
    fprintf(stderr, "call invalid!\n");
    exit(1); /* shall not happen */
  }

  return host;
}

char *is_v6(char *ip) {
  return strchr(ip, ':');
}

/* via http://stackoverflow.com/questions/13504934/binding-sockets-to-ipv6-addresses
   return the interface index (aka scope) of an ipv6 address, which is required
   in order to bind to it.
*/
unsigned get_v6_scope(const char *ip){
  struct ifaddrs *addrs, *addr;
    char ipAddress[NI_MAXHOST];
    uint32_t scope=0;
    int i;
    
    // walk over the list of all interface addresses
    getifaddrs(&addrs);
    for(addr=addrs;addr;addr=addr->ifa_next){
        if (addr->ifa_addr && addr->ifa_addr->sa_family==AF_INET6){ // only interested in ipv6 ones
            getnameinfo(addr->ifa_addr,sizeof(struct sockaddr_in6),ipAddress,sizeof(ipAddress),NULL,0,NI_NUMERICHOST);
            // result actually contains the interface name, so strip it
            for(i=0;ipAddress[i];i++){
                if(ipAddress[i]=='%'){
                    ipAddress[i]='\0';
                    break;
                }
            }
            // if the ip matches, convert the interface name to a scope index
            if(strcmp(ipAddress,ip)==0){
                scope=if_nametoindex(addr->ifa_name);
                break;
            }
        }
    }
    freeifaddrs(addrs);
    fprintf(stderr, "scope: %d\n", scope);
    return scope;
}

/* this is the contents of the makro IN6_IS_ADDR_LINKLOCAL,
   which doesn't compile, when used directly, for whatever reasons */
int is_linklocal(struct in6_addr *a) {
  return ((a->s6_addr[0] == 0xfe) && ((a->s6_addr[1] & 0xc0) == 0x80));
}

void host_dump(host_t *host) {
  fprintf(stderr, "host -   ip: %s\n", host->ip);
  fprintf(stderr, "       port: %d\n", host->port);
  fprintf(stderr, "       isv6: %d\n", host->is_v6);
  fprintf(stderr, "       size: %ld\n", host->size);
  fprintf(stderr, "        src: %p\n", host->sock);
}

void host_clean(host_t *host) {
  free(host->sock);
  free(host->ip);
  free(host);
}
