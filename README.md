[![Build Status](https://travis-ci.org/TLINDEN/udpxd.svg?branch=master)](https://travis-ci.org/TLINDEN/udpxd)
[![Build status](https://ci.appveyor.com/api/projects/status/0tujxsr8u324dv9v?svg=true)](https://ci.appveyor.com/project/TLINDEN/udpxd)

## UDPXD - A general purpose UDP relay/port forwarder/proxy

This is the README file for the network program udpxd.

udpxd can be used to forward or proxy UDP client traffic
to another port on another system. It also supports binding
to a specific ip address which will be used as the source
for outgoing packets. It supports ip version 4 and 6.

## Documentation

You can read the documentation without installing the
software:

    perldoc udpxd.pod

If it is already installed, you can read the manual page:

    man udpxd

## Installation

This software doesn't have eny external dependencies, but
you need either BSD make or GNU make installed to build it.

First you need to check out the source code. Skip this, if
you have already done so:

    git clone git@github.com:TLINDEN/udpxd.git

Next, change into the newly created directory 'udpxd' and
compile the source code:

    cd udpxd
    make

To install, type this command:

    sudo make install

This will install the binary to `$PREFIX/sbin/udpxd` and
the manual page to `$PREFIX/man/man1/udpxd.1`. You can
modify `$PREFIX` during installation time like this:

   make install PREFIX=/opt

## Getting help

Although I'm happy to hear from udpxd users in private email,
that's the best way for me to forget to do something.

In order to report a bug, unexpected behavior, feature requests
or to submit a patch, please open an issue on github:
https://github.com/TLINDEN/udpxd/issues.

## Copyright and license

This software is licensed under the GNU GENERAL PUBLIC LICENSE version 3.

## Authors

T.v.Dein <tom AT vondein DOT org>

## Project homepage

https://github.com/TLINDEN/udpxd
