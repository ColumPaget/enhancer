[![Build Status](https://travis-ci.com/ColumPaget/Enhancer.svg?branch=master)](https://travis-ci.com/ColumPaget/Enhancer)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

SYNOPSIS
=========

Enhancer is an LD_PRELOAD library that contains a number of configurable 'enhancements' that can be applied to add features to programs that lack them. It works by intercepting calls to common functions in libc or libx11 and running it's own code when called. Enhancements include setting various socket options, redirecting file opens, adding logging when a function is called, sending messages when a function is called, changing x11 fonts, adding socks4 and socks4a proxy support, etc.

Enhancements are configured in /etc/enhancer.conf (or other config file specified using an environment variable) and then the program is run using 'LD_PRELOAD=<path to enhancer.so> <progname>'

LICENSE
=======

Enhancer is released under the GPLv3 license.

AUTHOR
======

Enhancer is written by Colum Paget. All patches/bugreports/requests should be sent to colums.projects@gmail.com, or handled through the project's github page.


PROGRAMS THAT DON'T WORK WITH ENHANCER
======================================

Some programs will not work with enhancer. Enhancer relies on intercepting calls to shared libraries, so if a program is statically linked then enhancer will not be able to intercept function calls. Furthermore enhancer has been seen to cause issues for some complex programs, like the Pale Moon webbrowser, and also does not play well with emulators like wine. Finally, while enhancer likely does have some use for system monitoring or hardening, the user should be aware that there are methods of avoiding triggering enhancer functions, for instance by calling kernel syscalls directly rather than going through libc. Enhancer is primarily intended to add features or fix a few common types of bug, not really as a security tool, though it still has some value in that area.

For these reasons enhancer should not be rashly applied to all programs running on a system by adding it to /etc/ld.so.preload, as this may cause major system problems. A better solution is to use 'bash alias' functions like so:

```
alias mpg123='LD_PRELOAD=/usr/local/lib/enhancer.so mpg123'
```

The 'nodescend' action can be used to prevent a process from passing enhancer.so to its child processes in those situations where those child processes might be disrupted by having a preloaded library.

CONFIG FILE
===========

The enhancer config file contains entries of the form:

```
program <program list>
{
<function name> <match values> <actions>
}
```

for example:

```
program mpg123,mplayer
{
open path=*.mp3,*.ogg setbasename "track=%1" xtermtitle "$(progname) $(track)" send "unix:/tmp/nowplay.sock|$(track)"
onexit xtermtitle idle
}
```

Enhancer looks for its config in the following places:

1. path specified by `ENHANCER_CONFIG_FILE` environment variable.
2. `~/.enhancer/<progname>.conf`
3. `~/.enhancer.conf`
4. `/etc/enhancer.d/<progname>.conf`
5. `/etc/enhancer.conf`

in the case of the paths containing `progname` the name of the currently running program is subsitututed in before the config file is looked for.


RULE SYNTAX
===========

Each line in a config section consists of a function name (or name that identifies a group of functions), a set of match tests that apply to data passed to the function, and a list of actions to take. These things are processed in the order that they occur in the line.

Hence:

```
open path=/etc/passwd redirect /etc/fakeusers
```

Applies to the 'open' group of functions. This includes not just open, but open64, openat and fopen too. The match test 'path=/etc/passwd' specifies that this rule only applies when that path is opened. The action 'redirect' instructs that the file `/etc/fakeusers` should be opened instead of `/etc/passwd`.

Match tests include an operator  which can be one of '=', '!=', '=='



STRING QUOTING
==============

Enhancer recognizes two styles of quoting in its config file. Firstly via use of double-quotes, and secondly via use of backslash quoting.

```
	open path="/home/my directory/my file.txt" log "opened file %1"
	open path=/home/my\ directory/my\ file.txt log "opened file %1"
```




HOOKED FUNCTIONS
================

Enhancer only hooks a few useful libc functions. These are:

```
main       the 'main' function that's the entry point to the program. This is program start-up.
onexit     triggers on program exit
arg        not a function, triggers on every argument to the program
open       'open group' which included open, open64, openat, fopen
close
uname
unlink
rename
time       applies to both time and gettime of day
settime
setuid
setgid
connect
bind
listen
accept
fork       applies to fork and vfork
exec       applies to all 'exec' family functions
system
sysexec    applies to both system and exec
gethostip  applies to 'gethostbyname' and 'getaddrinfo'
chown
chmod
chdir
chroot
time
settime
select     applies to both select and poll
fsync
fdatasync
dlopen
dlclose
```

X11 Hooked Functions
====================

if X11 support is compiled in, the following functions can be hooked

```
XMapWindow 
XRaiseWindow
XLowerWindow
XSendEvent
XNextEvent
XLoadFont
XChangeProperty
```


MATCHES
=======

You can specify 'match modifiers' for a function. The config line will only be used if a function call matches these modifiers. Available modifiers are:


```
path          perform match against first arg of the function. This is usually a file path, but for 'connect' and 'bind' it can be a URL
basename      peform a match against the basename (leading directory removed) of the first arg of the function
family        for 'connect', 'bind' and 'accept' this is the url type. It can be 'ip4', 'ip6', 'net', or 'unix'. 'net' matches both 'ip4' and 'ip6'
peer          for 'connect' and 'accept' this is the remote host ip, extracted from 'path' which will be url
port          for 'connect' and 'bind' this will be the port to bind or connect to
user          match against username current process is running as
group         match against primary groupname current process is running as
arg           match if any arg in the programs arguments matches
```

The 'arg' match is a special case. You can use it to match against command-line arguments of the program. e.g.

```
bind arg=-local localnet
```


ACTIONS
=======

The following actions can be booked against a function, to be carried out when it is called.

```
deny            do not perform the function, return and error code indicating failure
pretend         do not perform the function, return and error code indicating success
allow           perform the function as expected
die             cause the program to exit
die-on-fail     cause the program to exit if function call fails
abort           raise abort signal, causing program to exit
collect         collect child processes (i.e. calls 'waitpid(-1)')
deny-symlinks   for file functions: do not operate on symlinks
setvar          set a variable. Takes an argument of the form 'name=value'
setenv          set environment variable. Takes an argument of the form 'name=value'
setbasename     set a variable with the basename of the first argument to this function
log             log to default logfile. Takes string argument.
syslog          log to syslog. Takes string argument.
syslogcrit      log a critical event to syslog. Takes string argument.
echo            write to standard out. Takes string argument.
debug           write to standard error. Takes string argument.
send            send to a url. Takes string argument in the form 'url|message'.
xtermtitle      set title of xterm compatible terminal. Takes a string argument.
exec            execute a program/command. Takes string argument.
sleep           sleep for seconds, takes numeric argument
usleep          sleep for nanoseconds, takes numeric argument
mlockall        lock process memory pages, and all future pages, so they are never swapped out
mlockcurr       lock process current memory pages so they are never swapped out
redirect        redirect main argument to a different value. Usually used to change file paths.
fallback        list of fallback arguments. Used with X11 fonts to specify fallbacks if font doesn't load.
searchpath      only for 'open'. List of directories to search for a file.
fdcache         only for 'open'. Use cached file descriptor for this file if one is already open
cmod            only for 'open'. Set file permissions for file create, takes octal 'permissions' argument
create          only for 'open'. Create file if it doesn't exist.
lock            only for 'open'. Lock file.
nosync          only for 'open'. Don't fsync this file.
fadv_seq        only for 'open'. Specify this file will be read sequentially (increases readahead)
fadv_rand       only for 'open'. Specify this file will be random access (no readahead)
fadv_nocache    only for 'close'. Don't cache this file (useful for logfiles etc)
qlen            only for 'listen'. Alter queue len, takes numeric argument.
sanitise        only for 'exec' and 'system'. Remove shell metacharacters from command string.
die-on-taint    only for 'exec' and 'system'. Exit program if shell metacharacters found in command string. 
deny-on-taint   only for 'exec' and 'system'. Refuse to launch program if shell metacharacters found in command string.
shred           only for 'unlink'. Overwrite file with random bytes.
backup          only for 'open' and 'close'. Backup 'file' to 'file-'
pidfile         create a pidfile, takes a string argument that is the file path
lockfile        create a lockfile, takes a string argument that is the file path
fsync           only for 'fdatasync'. use 'fsync' instead
fdatasync       only for 'fsync' use 'fdatasync' instead
keepalive       only for 'connect' and 'accept'. Enable keepalives on socket
localnet        only for 'connect' and 'accept'. Only connect to local hosts (DONTROUTE)
reuseport       only for 'bind'. Allow more than one server process to share same port
tcp-qack        only for 'connect' and 'accept'. enable TCP_QUICK_ACK
tcp-nodelay     only for 'connect' and 'accept'. disable Nagel's algorithm (TCP_NODELAY)
ttl             only for 'connect', 'bind' and 'accept'. set ttl for packets from this socket
freebind        only for 'bind'. bind to an ip address even if it doesn't currently exist
xstayabove      only for 'XMapWindow'. Set window to stay on top.
xstaybelow      only for 'XMapWindow'. Set window to stay below others.
xiconized       only for 'XMapWindow'. Set window iconized
xunmanaged      only for 'XMapWindow'. Set window to be unmanaged (borderless).
xfullscreen     only for 'XMapWindow'. Set window to be fullscreen.
xtransparent    only for 'XMapWindow'. Try to fake transparency (only works for simple windows).
xnormal         only for 'XMapWindow'. Set window to be 'normal' (not fullscreen, iconized, etc).
allow-xsendevent   only for 'XNextEvent'. Allow reception of events produced by XSendEvent even if program (e.g. xterm) doesn't usually allow that.
unshare         unshare (containers). Experimental. Takes string argument. See 'unshare' below.
getip           get IP address and store it in variable called 'ip:hostname'
cd              change directory. Takes string argument
chroot          chroot into current directory.
copyclone       clone (copy) file into current directory
linkclone       clone (hardlink) file into current directory
ipmap           only for 'gethostbyname/getaddrinfo'. Map name to 'fake' IP address. See 'IP MAPPING' below.
writejail       prefix all writes with the filepath given as a string argument (poor man's chroot)
nodescend       don't propagate enhancer.so to child processes
```

The 'pretend' action is only supported for the functions: bind, dlclose, unlink, unlinkat, fsync, fdatasync, fchown, chown, fchmod, chmod, chroot, fchroot.


ACTION ARGUMENTS
================

Some actions take a string as an argument. This string can contain the following substitutions.

```
%%      the '%' character
%f      the name of the called function
%1      the first argument of the called function (this is normally the 'most significant' argument, like the path for 'open')
%2      the second argument of the called function (this is function dependant, e.g. for open it's "read", "write", "read/write"
%n      program name
%N      program path
%p      program pid
%d      current working directory
%h      local hostname
%H      local hostname with domain
%T      time in %H:%M:%S format
%D      date in %Y:%m:%d format
%t<x>   time component. 'x' is a component as for strftime 'x'. So '%ta' is the weekday name '%tH' is hours, etc.
$(name) variable previously set with 'setvar'

```


REDIRECTIONS
============

The redirect action has numerous uses. It's main use is to change the file that's going to be opened in 'open'. It can also be used in the 'bind' function call to change the address that is being bound to, or in the 'connect' function call to change the node being connected to. 'connect' and 'bind' redirections have the form of a URL. Possible redirect URL types are:

```
socks:<user>@<host>:<port>      redirect to a socks4 or socks4a server (adequate for use with ssh -D proxying)
socks:<host>:<port>             redirect to a socks4 or socks4a server (adequate for use with ssh -D proxying)
tcp:<host>:<port>               redirect to a tcp host
unix:<path>                     redirect to a unix socket
 
```



IPMAPPING
=========

When programs connect to network hosts they call 'gethostbyname' or 'gethostaddr' to lookup the network address and then call 'connect' to connect to them. If using a 'connect' redirect to a socks 4a proxy, the hostname needs to be presevered across the function calls. The 'ipmap' action provides this. An entry of the form:

```
gethostip ipmap
```

Will map the hostname lookup to a false ipaddress in the form '0.0.0.x' When the program then calls 'connect' it will pass this fake IP address, which will be looked up to find the hostname and pass it to the socks proxy. So the full config for this becomes:

```
gethostip ipmap
connect path=tcp:* redirect socks:127.0.0.1:9090
```

It's a good idea to at least specify `path=tcp:` to prevent trying to redirect, say, a connection for syslog logging to socks. You can be more specific if you only want to map certain hosts. e.g. if local hosts are in the domain '.local' then we might use:

```
gethostip path!=*.local ipmap
connect path=tcp:0.* redirect socks:127.0.0.1:9090
```

The use of `path=tcp:0.*` in this case ensures that only IP addresses that have been mapped with ipmap are redirected to socks. The use of `path!=*.local` in the 'gethostip' rule means that local addresses are not ipmapped.


UNSHARE
=======

'unshare' is an action that uses the linux namespaces 'unshare' function. It takes a string argument that lists the namespace types to 'unshare' from the main namespace. Possible values are 'user', 'net', 'uts', 'ipc', and 'mount'. 'user' can be used to allow 'chroot' for users other than root, like so:

```
program jailthis
{
main unshare user cd /tmp chroot
}
```

unsharing the net namespace will cause the program to be unable to see real network interfaces, so preventing network access.

unsharing 'uts' allows setting a hostname that the app will see using 'host=' like so

```
program jailthis
{
main unshare uts,host=myhost
}
```

namespace types can be combined like so:

```
program jailthis
{
main unshare user,net,uts,host=myhost
}
```


USE CASES
=========

## Echo debugging to terminal

Sometimes you might want to echo some debugging to the terminal.

```
program myprogram.exe
{
open debug "opening %1"
connect debug "connecting to %1"
unlink debug "unlinking %1"
}
```

## Update terminal title bar 

xterm compatible x11 terminals (which is most of them) support sending an escape sequence that changes the title of the terminal window. The 'xtermtitle' action supports doing this.

```
program mpg123,mplayer,alsaplayer
{
open path=*.mp3,*.ogg,*.flac xtermtitle "playing: %1"
atexit xtermtitle "playing nothing"
}
```


## Log certain functions

Sometimes you might want to add logging to certain function calls. You can achive this using the 'syslog', 'syslogcrit' and 'log' actions. These actions take an argument that is the string you want to log. For example:

```
program suspect.exe
{
open path=/etc/* syslog "$(progname) tried to open %1 in function %f"
}
```

Will syslog a message if the program tries to open a file in /etc


## Make file contents unrecoverable on delete (shred/secure delete)

The 'shred' action overwrites a file with random bytes multiple times to ensure it's contents are completely unrecoverable. Obviously, care should be taken in using this function!

```
program paranoid.exe
{
unlink path=*/temp_secret.txt shred
}
```

NB: SHRED WILL NOT OPERATE ON SYMLINKS OR FILES THAT HAVE MORE THAN ONE HARDLINK TO THEM.


## Run a program when an upload ends

Sometimes it's desirable to be able to run an importer program when a file is uploaded to an FTP or other type of fileserver. This can be achieved using the 'exec' action.

```
program ftpd,pure-ftpd
{
close path=/uploads/*.csv exec "import-csv.exe %1"
}
```


## Fix leaky file descriptors

Some programs fail to close files when they're done with them, and reopen them again later. You can fix this with the 'fdcache' action.

```
program leaker.exe
{
open fdcache
} 
```

Enhancer internally caches file descriptors, and the fdcache function tells it to reuse any open file descriptor that matches the path and open-type (read or write) of a call to a function in the 'open' family of functions. The file descriptor is rewound to the start of the file using 'fseek' unless the call is made with the `O_APPEND` flag;

Care should be taken when using this action, as if a program intentionally opens the same file twice for different uses, it may cause unexpected behavior. For this reason you may want to only apply fdcache to certain file opens, like so:

```
program leaker.exe
{
open path=/etc/hosts fdcache
}
```

## Fix programs that fail to collect child processes

Sometimes you get programs that fork or spawn other programs, but fail to clean up/collect their exited child processes, resulting in a swarm of zombies. The 'collect' action can help with this.

```
program forker
{
select collect
}
```

## Exit if certain function calls fail

Many function calls can be set to exit if they fail by using the 'die-on-fail' action. This is mostly useful for 'setuid' and 'setgid' if a program fails to switch user and may still be running as root, and 'chdir' and 'chroot' if it fails to switch directory and may be working in the wrong directory.

```
program dangerous
{
setuid die-on-fail 
}
```

## Pause a program at a certain point

Sometimes it's useful to pause a program. For instance, it maybe useful to attach gdb to a child process of a server (e.g. a cgi program being run from a webserver) before the process fully runs. This can also be useful when debugging dynamically loaded libraries like enhancer.so, because gdb does not always seem to know what libraries were linked in when a coredump was produced, but if you can hook gdb to the running process, then it does. Pausing a program can be achieved using the 'sleep' or 'usleep' functions. 'sleep' takes an argument of seconds, and 'usleep' takes an argument of nanoseconds


```
program crashing.cgi
{
main sleep 30
}
```

## Fix filewrites that fail if the file does not exist

Sometimes code is written that saves data to a file, but expects the file to exist and fails when it doesn't. If you don't want that behavior you can use the 'create' action for the 'open' function

```
program contacts
{
open basename=contacts.txt create
}
```

## Fix files created with bad permissions

The 'open' function call takes a third argument that specifies the permissions of created files. Sometimes this is overlooked by developers and files get created with random permissions. The 'cmod' action can be used to specify the permissions of the created file.

```
program contacts
{
open basename=contacts.txt cmod 0660
}
```


## Notify kernel of planned file usage

You can inform the kernel when opening or closing files of how you plan to use those files. `fadv_seq` implies the file is going to be read in a sequential, linear fashion, and the kernel will generally read more of the file into it's cache assuming it will be needed on a future read. `fadv_rand` implies that the file is going to be opened for random access, and so isn't read into cache. `fadv_nocache` is used when closing a file to notify the kernel that this file isn't likely to be read again in the near future (this is particularly useful for logfiles).

```
program myserver
{
open path=/var/data.db fadv_rand
open path=/etc/motd.txt fadv_seq
close path=/var/log/myserver.log fadv_nocache
}  
```

## Add a pidfile to programs that lack them

If you have a program that doesn't produce a pidfile, and you wish it did, you can add the feature using the 'pidfile' action.

```
program myserver
{
main pidfile /var/run/myserver.pid
}
```

## Add a lockfile to programs that lack them

Don't confuse this with 'lock' (see below) this creates a new file and locks it. 'lockfile' can be used in any function config.

```
program myserver
{
main lockfile /var/lock/myserver.lck
}
```

## Lock files that a program opens, but doesn't lock

Don't confuse this with 'lockfile' (see above) this locks a file that the program already opens. Therefore 'lock' can only be used in teh config of the 'open' function call.

```
program myserver
{
open path=/var/data.db lock
}
```

## Add socket options

Enhancer can add a number of socket options during the connect, bind and accept system calls. The actions for these are:

```
keepalive      enable socket keepalives so that the program can detect if a peer dies
localnet       don't route - only allow connections to/from hosts on the local network
reuseport      allow more than one server to connect to the same port, and load-balance between them
tcp-qack       TCP quick ack
tcp-nodelay    TCP nodelay: disable Nagel's algothrim
ttl            Set TTL for packets. Can be used to limit distance packets can travel through network.
freebind       Bind to an IP address even if it's not currently existing. When network device comes up with this IP, start using it.
```

## Bind only to a certain IP

For programs that don't support this function enhancer can be used to force binding to only a certain IP address

```
program simple-server
{
bind family=ip4 redirect 127.0.0.1
}
```

## Modify listen queue length

The 'listen' syscall has an argument that specifies how many connections can be queued up waiting to be accepted. The value for this is often rather randomly selected, as it is dependant on how many connections an application will be expecting to have to service. The 'qlen' action (which exists solely for 'listen') can be used to set this value.

```
program simple-server
{
listen qlen=100
}
```

## Per application firewall.

The 'accept' system call can be modified to only accept connections from certain hosts. This could be useful to protect sensitive services from unauthorized hosts in those situations where someone has autority over a service, but not over the host firewall. Also useful if you worry about making mistakes in your main firewall and letting things through by accident.

```
program test-server
{
accept peer!=192.168.5.* deny
}
```

## Limit commands that can be 'exec' or 'system' from a program

Enhancer can lock down what commands/programs can be launched from within a program. Consider a program 'pinger' which launches 'ping' and shouldn't launch anything else:

```
program pinger
{
exec basename!=ping deny
system basename!=ping deny
}
```

This can be simplified down to one line using the 'sysexec' fake function name, which applies its config to both system and exec:

```
program pinger
{
sysexec basename!=ping deny
}
```

However, if a program is compromised then there will be ways around this check, so overriding system and exec is more useful for...


## Sanitise 'system' and 'exec' arguments to prevent command injection

Often web applications and other programs that run commands using system can be tricked into running other commands using shell metacharacters. The 'sanitise', 'die-on-taint' and 'deny-on-taint' actions exist to help with this:

```
sanitise            remove shell metacharacters from a command, and then run it
deny-on-taint       refuse to run a command that contains shell metacharacters
die-on-taint        exit program if it tries to run a command that contains shell metacharacters
```

for example, the below config will only allow 'pinger' to run 'ping' and will refuse if the 'ping' command contains any shell metacharacters.


```
program pinger
{
sysexec basename!=ping deny 
sysexec deny-on-taint
}
```



## Change fonts in X11 programs that have them hardcoded.

Some simple X11 programs tend to hardcode their fonts rather than having those be configurable. This can be changed with the 'redirect' and 'fallback' actions. 'redirect' forces the use of the specified font, whereas 'fallback' specifies a font to be used if the font requested by the program wasn't found. Both 'redirect' and 'fallback' can take a comma-separated list of fonts, and will use the first of these that loads successfully.

An example of this is the old program 'mouseclock' which turns your mouse pointer into a clock when you move it to the root desktop window. This can be configured like so:

```
program mouseclock
{
XLoadFont path=-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-* fallback kates,-xos4-terminus-medium-r-normal--0-0-72-72-c-0-iso8859-1,fixed
}
```


## Make dlopen libraries compatible with valgrind

Valgrind is a utility that checks for various memory errors. Unfortunately it has an issue with libraries/plugins that are loaded with dlopen. Valgrind doesn't map function addresses to function names until the program exits, but unfortunately by then some libraries that were opened with dlopen may have been unloaded from memory by dlclose, producting valgrind output that does not know which functions were called. This can be solved using the 'pretend' action against the 'dlclose' function, like so:

```
program lua
{
dlclose pretend
}
```

This will prevent dlclose from being called at program exit, and thus function call names should be output correctly from valgrind.
