[![status-badge](https://ci.codeberg.org/api/badges/15646/status.svg)](https://ci.codeberg.org/repos/15646)
[![License](https://img.shields.io/badge/license-GPL-blue.svg)](https://codeberg.org/scip/udpxd/raw/branch/main/LICENSE)

> [!CAUTION]
> This software is now being maintained on [Codeberg](https://codeberg.org/scip/udpxd/).

## UDPXD - A general purpose UDP relay/port forwarder/proxy

This is the README file for the network program udpxd.

## Introduction

There are cases, when you need to change the source ip address of a
client, but can't do it in the kernel. Either because you don't have a
firewall  running or because it doesn't work, or because there's no
firewall available.

In such cases the usual aproach is to use a port forwarder. This is a
small piece of software, which opens a port on the inside, visible to
the client and establishes a new connection to the intended
destination, which is otherwise unreachable for the client. In fact, a
port forwarder  implements hiding nat or masquerading in
userspace. Tools like this are also usefull for testing things, to
simulate requests when the tool you need to use for the test are
unable to set their source ip address (binding address).

For   TCP    there   are    LOTs   of   solutions    available,   e.g.
[tcpxd](http://quozl.us.netrek.org/tcpxd/).  Unfortunately  there  are
not so many solutions available if you want to do port forwarding with
UDP.           One          tool         I          found          was
[udp-redirect](http://www.brokestream.com/udp_redirect.html).   But it
is just a  dirty hack and didn't  work for me.  Also  it doesn't track
clients  and  is  therefore  not   able  to  handle  multiple  clients
simultaneusly.

So I  decided to  enhance it.  The  - current -  result is  `udpxd`, a
"general purpose UDP relay/port forwarder/proxy". It supports multiple
concurrent clients, it is able to bind to a specific source ip address
and it is small and simple.

udpxd can be used to forward or proxy UDP client traffic
to another port on another system. It also supports binding
to a specific ip address which will be used as the source
for outgoing packets. It supports ip version 4 and 6.

## Quick Start

How does it work: I'll explain it on a concrete example. On the server
where  this  website  is  running  there  are  multiple  ip  addresses
configured  on  the outside  interface,  because  I'm using  jails  to
separate services. One of those jail ip addresses is 78.47.130.33. Now
if I want to send a  DNS query to the Hetzner nameserver 213.133.98.98
from the root  system (not inside the jail with  the .33 ip address!),
then the  packet will go out  with the first interface  address of the
system. Let's say I don't want that (either for testing or because the
remote  end does  only  allow me  to  use the  .33  address). In  this
szenario I could use **udpxd** like this:

```default
udpxd -l 172.16.0.3:53 -b 78.47.130.33 -t 213.133.98.98:53
```

The  ip address  172.16.0.3 is  configured on  the loopback  interface
lo0. Now I can use dig to send a dns query to 172.16.0.3:53:

```default
dig +nocmd +noall +answer google.de a @172.16.0.3
google.de.              135     IN      A       173.194.116.151
google.de.              135     IN      A       173.194.116.152
google.de.              135     IN      A       173.194.116.159
google.de.              135     IN      A       173.194.116.143
```

When we look with tcpdump on our external interface, we see:

```default
IP 78.47.130.33.24239 > 213.133.98.98.53: 4552+ A? google.de. (27)
IP 213.133.98.98.53 > 78.47.130.33.24239: 4552 4/0/0 A 173.194.116.152,
              A 173.194.116.159, A 173.194.116.143, A 173.194.116.151 (91)
```

And this is how the same request looks on the loopback interface:

```default
IP 172.16.0.3.24239 > 172.16.0.3.53: 4552+ A? google.de. (27)
IP 172.16.0.3.53 > 172.16.0.3.24239: 4552 4/0/0 A 173.194.116.152,
              A 173.194.116.159, A 173.194.116.143, A 173.194.116.151 (91)
```

As you can  see, dig sent the packet to  172.16.0.3:53, udpxd took it,
opened  a new  outgoing socket,  bound  to 78.47.130.33  and sent  the
packet with that source ip address  to the hetzner nameserver. It also
remembered which  socket the particular  client (source ip  and source
port) has used.  Then the response came back from  hetzner, udpxd took
it, looked up its internal cache  for the matching internal client and
sent it back to it with the source ip on the loopback interface.

As I already  said, udpxd is a general purpose  udp port forwarder, so
you can  use it with almost  any udp protocol. Here's  another example
using ntp:  now I set up  a tunnel between two  systems, because udpxd
cannot bind to any interface with port 123 while ntpd is running (ntpd
binds to *.123). So on system A I have 3 udpxd's running:

```default
udpxd -l 172.17.0.1:123 -b 78.47.130.33 -t 178.23.124.2:123
udpxd -l 172.17.0.2:123 -b 78.47.130.33 -t 5.9.29.107:123
udpxd -l 172.17.0.3:123 -b 78.47.130.33 -t 217.79.179.106:123
```

Here I  forward ntp queries on  172.16.0.1-3:123 to the real  ntp pool
servers and I'm  using the .33 source ip to  bind for outgoing packets
again. On the other system B, which is able to reach 172.17.0.0/24 via
the tunnel, I reconfigured the ntpd like so:

```default
server 172.17.0.1 iburst dynamic
server 172.17.0.2 iburst dynamic
server 172.17.0.3 iburst dynamic
```

After restarting, let's look how it works:

```default
ntpq> peers
     remote           refid      st t when poll reach   delay   offset  jitter
==============================================================================
*172.17.0.1      131.188.3.221    2 u   12   64    1   18.999    2.296   1.395
 172.17.0.2      192.53.103.104   2 u   11   64    1    0.710    1.979   0.136
 172.17.0.3      130.149.17.8     2 u   10   64    1   12.073    5.836   0.089
```

Seems to work :), here's what we see in the tunnel interface:

```default
14:13:32.534832 IP 10.10.10.1.123 > 172.17.0.1.123: NTPv4, Client, length 48
14:13:32.556627 IP 172.17.0.1.123 > 10.10.10.1.123: NTPv4, Server, length 48
14:13:33.535081 IP 10.10.10.1.123 > 172.17.0.2.123: NTPv4, Client, length 48
14:13:33.535530 IP 172.17.0.2.123 > 10.10.10.1.123: NTPv4, Server, length 48
14:13:34.535166 IP 10.10.10.1.123 > 172.17.0.1.123: NTPv4, Client, length 48
14:13:34.535278 IP 10.10.10.1.123 > 172.17.0.3.123: NTPv4, Client, length 48
14:13:34.544585 IP 172.17.0.3.123 > 10.10.10.1.123: NTPv4, Server, length 48
14:13:34.556956 IP 172.17.0.1.123 > 10.10.10.1.123: NTPv4, Server, length 48
14:13:35.535308 IP 10.10.10.1.123 > 172.17.0.2.123: NTPv4, Client, length 48
14:13:35.535742 IP 172.17.0.2.123 > 10.10.10.1.123: NTPv4, Server, length 48
14:13:36.535363 IP 10.10.10.1.123 > 172.17.0.1.123: NTPv4, Client, length 48
```

And the forwarded traffic on the public interface:

```default
14:13:32.534944 IP 78.47.130.33.63956 > 178.23.124.2.123: NTPv4, Client, length 48
14:13:32.556586 IP 178.23.124.2.123 > 78.47.130.33.63956: NTPv4, Server, length 48
14:13:33.535188 IP 78.47.130.33.48131 > 5.9.29.107.123: NTPv4, Client, length 48
14:13:33.535500 IP 5.9.29.107.123 > 78.47.130.33.48131: NTPv4, Server, length 48
14:13:34.535255 IP 78.47.130.33.56807 > 178.23.124.2.123: NTPv4, Client, length 48
14:13:34.535337 IP 78.47.130.33.56554 > 217.79.179.106.123: NTPv4, Client, length 48
14:13:34.544543 IP 217.79.179.106.123 > 78.47.130.33.56554: NTPv4, Server, length 48
14:13:34.556932 IP 178.23.124.2.123 > 78.47.130.33.56807: NTPv4, Server, length 48
14:13:35.535379 IP 78.47.130.33.22968 > 5.9.29.107.123: NTPv4, Client, length 48
14:13:35.535717 IP 5.9.29.107.123 > 78.47.130.33.22968: NTPv4, Server, length 48
14:13:36.535442 IP 78.47.130.33.24583 > 178.23.124.2.123: NTPv4, Client, length 48
```

You see, the ntp server gets the time via the tunnel via udpxd which
in turn forwards it to real ntp servers.

Note, if you left the parameter -b out, then the first ip address of
the outgoing interface will be used.

Udpxd can also proxy ipv6 udp traffic and forward between protocols
(v4 to v6 and vice versa). For instance listen on ipv6 loopback and
forward to ipv6 google:

```default
udpxd -l [::1]:53 -t [2001:4860:4860::8888]:53
```

Listen on ipv6 loopback and forward to ipv4 google:

```default
udpxd -l [::1]:53 -t 8.8.8.8:53
```

Or listen on ipv4 loopback and forward to ipv6 google:

```default
udpxd -l 127.0.0.1:53 -t [2001:4860:4860::8888]:53
```

Of course it is possble to set the bind ip address (-b) using ipv6 as
well.



## Documentation

You can read the documentation without installing the
software:

    perldoc udpxd.pod

If it is already installed, you can read the manual page:

    man udpxd

## Installation using pre-built binaries

You can download a statically pre-compiled binary for linux from the [release page](releases).

## Installation from source

This software doesn't have any external dependencies, but
you will  need `meson` and `ninja`: https://mesonbuild.com/.

Check out the source and execute:

```default
meson setup build
ninja -C build
```

You can supply some additional parameters to meson,
type `meson configuration` for details.

To install, execute:

```default
sudo ninja -C install
```

 ## Getting help

Although I'm happy to hear from udpxd users in private email,
that's the best way for me to forget to do something.

In order to report a bug, unexpected behavior, feature requests
or to submit a patch, [please open an issue](https://codeberg.org/scip/udpxd/issues).

## Copyright and license

This software is licensed under the GNU GENERAL PUBLIC LICENSE version 3.

## Authors

T.v.Dein <tom AT vondein DOT org>

## Project homepage

https://codeberg.org/scip/udpxd
