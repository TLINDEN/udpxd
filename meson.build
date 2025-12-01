# -*-python-*-

project(
        'udpxd',
        'c',
        license: 'GPL',
        version: '0.0.4',
        meson_version: '>=1.3',
        default_options: [
                'warning_level=2',
                'werror=true',
        ],
)

add_project_arguments(
        [
                '-Wno-unused-parameter',
                '-Wno-unused-result',
                '-Wno-missing-braces',
                '-Wno-format-zero-length',
                '-Wvla',
                '-Wno-sign-compare',
                '-Wno-narrowing',
                '-Wno-stringop-truncation'
        ],
        language: 'c',
)


c = meson.get_compiler('c')
conf = configuration_data()


# check for funcs.
foreach func : ['getopt', 'malloc', 'fprintf', 'strncpy', 'strlen', 'strtok', 'strchr', 'signal',
                'select', 'free', 'perror', 'getsockname', 'setegid', 'seteuid', 'syslog',
                'va_start', 'va_end', 'inet_ntop', 'getifaddrs', 'getnameinfo', 'ntohs', 'memcpy', 'memset', 'sprintf' ]
    conf.set('HAVE_'+func.to_upper(), c.has_function(func, prefix : '#include <unistd.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <signal.h>\n#include <sys/socket.h>\n#include <syslog.h>\n#include <stdarg.h>\n#include <arpa/inet.h>\n#include <sys/types.h>\n#include <ifaddrs.h>\n#include <netdb.h>\n'))
endforeach


# check commandline options
prefix = get_option('prefix')

if get_option('buildtype') == 'debug'
    conf.set('DEBUG', '1')
endif



# setup conf map
version = '@0@'.format(meson.project_version())
conf.set('prefix', prefix)
conf.set('VERSION', version)


# write out the config header
m = configure_file(
  input : 'platform.h.in',
  output : 'platform.h',
  configuration : conf,
)


# code
udpxd_sources = files(
    'client.c',
    'host.c',
    'log.c',
    'net.c',
    'udpxd.c'
)



executable(
        'udpxd',
        [udpxd_sources],
        install: true
)

# build manual page
pod2man = find_program('pod2man', native: true)
if pod2man.found()
  res = run_command(pod2man.full_path(), 'udpxd.pod', 'udpxd.1', check:true)
  if res.returncode() == 0
    install_man('udpxd.1')
  endif
endif
