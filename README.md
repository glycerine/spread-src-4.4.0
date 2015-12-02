Considering Paxos or Raft, but worried they will be too slow? Why you should learn about virtual synchrony instead.
-----------------

"[Users have] included the New York and Swiss Stock Exchange, the French Air Traffic Control System, the US Navy AEGIS, dozens of telecommunications provisioning systems, the control system of some of the world’s largest electric and gas grid managers, and all sorts of financial applications."
  -- Ken Birman, distributed systems professor, Cornell Computer Science Dept.


"To give some sense of the relative speed, experiments with 4-node replicated variables undertaken on the Isis and Horus systems in the 1980s suggested that virtual synchrony implementations in typical networks were about 100 times faster than state-machine replication using Paxos, and about 1000 to 10,000 times faster than full-fledged transactional one-copy-serializability." -- https://en.wikipedia.org/wiki/Virtual_synchrony


The following paragraphs are from Ken Birman's "A History of the Virtual Synchrony Replication Model". https://www.cs.cornell.edu/ken/history.pdf The emphasis and section headlines are mine.

## why this radically faster protocol is a little known underdog

"With the benefit of hindsight, one can look back and see that the convergence of the field around uncoordinated end‐system based failure detection enshrined a form of inconsistency into the core layers of almost all systems of that period. This, in turn, drove developers towards quorum‐based protocols, which don't depend on accurate failure detection – they obtain fault-tolerance guarantees by reading and writing to quorums of processes, which are large enough to overlap. Yet as we just saw, such protocols also require a two phase structure, because participants contacted in the first phase don’t know yet whether a write quorum will actually be achieved. Thus, one can trace a line of thought that started with the end‐to‐end philosophy, became standardized in TCP and RPC protocols, and ultimately compelled most systems to adopt quorum‐based replication. **Unfortunately, quorum‐based replication is very slow when compared with unreliable UDP multicast**, and this gave fault-tolerance a bad reputation. The **Isis protocols, as we’ve already mentioned, turned out to do well in that same comparison.**

## why virtual synchrony is faster: combined with locking, reads to your replicas can become radically faster (as they will take only one network roundtrip).

"The key insight here is that **within a virtual synchrony system, the group view represents a virtual world that can be "trusted"**. In the event of a partitioning of the group, processes cut off from the majority might succeed in initiating updates (for example if they were holding a lock at the time the network failed), but would be unable to commit them – the 2‐phase protocol would need to access group members that aren’t accessible, triggering a view change protocol that would fail to gain majority consent. Thus **any successful read will reflect all prior updates**: committed ones by transactions serialized prior to the one doing the read, plus pending updates by the reader’s own transaction. From this we can prove that our protocol achieves one‐copy serializability when running in the virtual synchrony model. And, as noted, **it will be dramatically faster than a quorum algorithm achieving the identical property.**

"This may seem like an unfair comparison: databases use quorums to achieve serializability. But in fact **Isis groups, _combined with locking_, also achieve serializability. Because the group membership has become a part of the model, virtually synchronous locking and data access protocols guarantee that any update would be applied to all replicas and that any read‐locked replica reflects all prior updates.** In contrast, because quorum‐based database systems lack an agreed‐upon notion of membership, to get the same guarantees in the presence of faults, a read must access two or more copies: a read quorum. Doing so is the only way to be sure that any read will witness all prior updates.

"**Enabling applications to read a single local replica as opposed to needing to read data from two or more replicas, may seem like a minor thing. But an application that can trust the data on any of its group members can potentially run any sort of arbitrary read‐oriented computation at any of its members.** A group of three members can parallelize the search of a database with each member doing 1/3 of the work, or distribute the computation of a costly formula, and the code looks quite normal: the developer builds any data structures that he or she likes, and accesses them in a conventional, non‐distributed manner. In contrast, application programmers have long complained about the costs and complexity of coding such algorithms with quorum reads. Each time the application touches a data structure, it needs to pause and do a network operation, fetching the same data locally and from other nodes and then combining the values to extract the current version. Even representing data becomes tricky, since no group member can trust its own replicas. Moreover, **whereas virtually synchronous code can execute in straight‐line fashion without pausing, a quorum‐ read algorithm will be subjected to repeated pauses while waiting for data from remote copies.**

"**Updates become faster, too.** In systems where an update initiator doesn’t know which replicas to "talk to" at a given point in time, there isn’t much choice but to use some kind of scatter‐shot approach, sending the update to lots of replicas but waiting until a have quorum acknowledged the update before it can be safely applied. Necessarily, such an update will involve a 2PC (to address the case in which a quorum just can’t be reached). In virtual synchrony, an update initiated by a group member can be sent to the "current members", and this is a well‐defined notion.

"Performance for cbcast can be astonishingly good: running over UDP multicast, this primitive is almost as fast as unreliable UDP multicast [14]. By using cbcast to carry updates (and even for locking, as discussed in [37]), we accomplished our goal, which was to show that one could guarantee strong reliability semantics in a system that achieved performance fully comparable to that of Zwaenepoel's process groups running in the V system."


## What is this repo? Well, spread-src-4.4.0, the code here, implements virtual synchrony!


From the humble http://www.spread.org/: 

> The Spread toolkit provides a high performance messaging service that is resilient to faults across local and wide area networks.
Spread functions as a unified message bus for distributed applications, and provides highly tuned application-level multicast, group communication, and point to point support. Spread services range from reliable messaging to fully ordered messages with virtual synchrony delivery guarantees.

## From the spread-src-4.4.0.tar distribution's Readme.txt:

~~~
SPREAD: A Reliable Multicast and Group Communication Toolkit
-----------------------------------------------------------

/=============================================================================\
| The Spread Group Communication Toolkit - Accelerated Ring Research Version  |
| Copyright (c) 1993-2014 Spread Concepts LLC                                 |
| All rights reserved.                                                        |
|                                                                             |
| The Spread package is licensed under the Spread Open-Source License.        |
| You may only use this software in compliance with the License.              |
| A copy of the license can be found at http://www.spread.org/license         |
|                                                                             |
| This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF       |
| ANY KIND, either express or implied.                                        |
|                                                                             |
| Spread is developed at Spread Concepts LLC with the support of:	      |
|    The Distributed Systems and Networks Lab, Johns Hopkins University       |
|                                                                             |
| Creators:                                                                   |
|    Yair Amir               yairamir@cs.jhu.edu                              |
|    Michal Miskin-Amir      michal@spreadconcepts.com                        |
|    Jonathan Stanton        jonathan@spreadconcepts.com                      |
|    John Schultz            jschultz@spreadconcepts.com		      |
|                                                                             |
| Major Contributors:                                                         |
|    Amy Babay               babay@cs.jhu.edu - accelerated ring protocol     |
|    Ryan Caudy              rcaudy@gmail.com - contribution to process groups|
|    Claudiu Danilov	     claudiu@acm.org - scalable, wide-area support    |
|    Cristina Nita-Rotaru    crisn@cs.purdue.edu - GC security                |
|    Theo Schlossnagle       jesus@omniti.com - Perl, autoconf, old skiplist  |
|    Dan Schoenblum          danschoenblum@gmail.com - Java interface.        |
|                                                                             |
| Contributors:                                                               |
|    Juan Altmayer Pizzorno juan@port25.com - performance and portability     |
|    Jacob Green            jgreen@spreadconcepts.com - Windows support       |
|    Ben Laurie	            ben@algroup.co.uk - FreeBSD port and warning fixes|
|    Daniel Rall            dlr@finemaltcoding.com - Java & networking fixes, |
|                                               configuration improvements    |
|    Marc Zyngier                        - Windows fixes                      |
|                                                                             |
| Special thanks to the following for discussions and ideas:                  |
|    Ken Birman, Danny Dolev, Mike Goodrich, Michael Melliar-Smith,           |
|    Louise Moser, David Shaw, Gene Tsudik, Robbert VanRenesse.               |
|                                                                             |
| Partial funding provided by the Defense Advanced Research Projects Agency   |
| (DARPA) and The National Security Agency (NSA) 2000-2004. The Spread        |
| toolkit is not necessarily endorsed by DARPA or the NSA.                    |
|                                                                             |
| WWW    : http://www.spread.org  and  http://www.spreadconcepts.com          |
| Contact: info@spreadconcepts.com                                            |
|                                                                             |
| Version 4.4.0 built 27/May/2014                                             |
\=============================================================================/

May 27, 2014 Ver 4.4.0
----------------------

No changes versus 4.4.0 RC2.

May 19, 2014 Ver 4.4.0 RC2
--------------------------

Improvements:
  - Slightly improved MSVS project files -- will work with MSBuild 4.0 (e.g. - .NET 4.0, MSVS 2010) now. Later versions of MSVS can auto-upgrade solution (thanks to Johann Lochner)
  - Remove named pipe for accepting UNIX socket connections on exit
  - Allow token sizes up to 64KB (uses IP fragmentation may exacerbate loss -- will log a warning) rather than hard failure if > 1.5KB
  - Send bcast retransmissions immediately rather than queue to suppress unnecessary re-requests in accelerated protocol

Bug fixes:
  - Reset Commit_set to only contain this daemon after installing a regular membership
  - Improved validity check for FORM1 tokens to either process or swallow
  - Only process FORM2 token that corresponds to most recent FORM1 token
  - Block delivery of < AGREED msgs until GROUPS algorithm is completed
  - Consult Last_discarded rather than Aru for deciding whether we have a packet already or not (Prot_handle_bcast)
  - Handle receiving packet after FORM1 processed and it is still listed as a hole on FORM2

January 22, 2014 Ver 4.4.0 RC1
------------------------------
Features:

 - New accelerated ring protocol tailored for data center networks.
   This protocol provides 30%-50% higher throughput and 20-35% lower latency 
   in modern local area networks. Both the original protocol and the accelerated
   ring protocol are available in this version.

 - More efficient packet packing.

 - Windows project files now build again.

 - New spread-service project to make Spread into a proper Windows service.
	- Needs testing from the community!

 - Additional and improved MEMBERSHIP and PROTOCOL logging.

Bug fixes:

 - EVS bugs:
	- Set Aru to 0 when we transition to EVS.
	- Ignore Token->seq when in EVS.
	- Fixed Backoff_membership referring to wrong packet when ring breaks in EVS.
	- Fixed token retransmissions while transitioning from EVS to OP.

 - Token bugs:
	- Token->aru calculaton fixed; Set_aru eliminated.
	- Ignore tokens from wrong membership + sender.

 - Alarm on Windows:
	- Alarm(EXIT) now exits with non-zero code instead of aborting.
	- Fixed long log lines crash bug.

 - Allow ports >= 2^15 to be used.

 - Turned off Nagle algorithm in Java library.

July 2, 2013 Ver 4.3.0 Final
----------------------------

June 11, 2013 Ver 4.3.0 Final
-----------------------------
Features:
 - Reload flow control parameters when spread.conf file is reloaded. 

April 4, 2013 Ver 4.3.0 RC3
----------------------------
Features:
 - Updated copyright

March 25, 2013 Ver 4.3.0 RC1
----------------------------
Features:

 - C library supports using a single <port> value as the spread name to connect 
to for all platforms when the daemon is on the same host as the client. 
On Unix that uses a Unix domain socket which should have the best performance, 
on Windows it now automatically converts this to a TCP connection to localhost. 
So applications can now safely use just <port> for any local connection and they 
will get the best option for each platform.

 - Improve default Membership timers to react faster to changes. 

 - Improve default flow control parameters to allow higher throughput.

 - Improve performance for multicast LANs by setting socket options so each daemon 
does not get it's own data messages back. 

 - Change the way we look for the local daemon in the configuration file when no 
daemon name is specified.  We now consult both the addresses returned by 
gethostbyname(gethostname()) and by directly queryin

 - Rework C client library handling of send/receive mutexes to eliminate blocking 
on Windows when lots of connections are made to a daemon and simplify the logic 
for all platforms. 

 - Export membership timeouts and flow control parameters to configuration file 
so that they can be changed at runtime. 

 - For Java library, add an isConnected() method. 

Bug Fixes:

 - Prevent frequent log message from being reported every time a non-faulty event 
was modified. This should remove a common source of the daemon flooding the log file. 

 - Change configure to actually test clock_gettime(CLOCK_MONOTONIC) works, rather 
than just that it compiles.

 - Change to stdutil configure to append onto CFLAGS rather than replacing it.

 - Remove optimization of long Lookup_timeout for single segment configurations.

 - Performance bug fix to protocol.c to only decrement retrans_allowed when we 
actually request a retransmission.

SOURCE INSTALL:
---------------

The source install uses autoconf and make to support many Unix and unix-like 
platforms as well as Windows 95+.

Windows is supported by a set of Visual Studio project files in
the win32 subdirectory. These files build the daemon, libraries, and user
programs.

Any not-specifically supported platform that has reasonably close to normal 
sockets networking should also  be easily portable. See the file PORTING for 
hints on porting.

From the directory where you unpacked the Spread source distribution do 
the following:

1) Run "./configure"

2) Run "make"

3) Run "make install" as a user with rights to write to the selected
installation location. (/usr/local/{bin,man,sbin,include,lib} by default)

4) Create a spread.conf file appropriate to your local configuration. For 
more info on how to do this look at the sample file "sample.spread.conf"
in docs or below in the binary install instructions.

5) To build the java library and documentation, see the Readme.txt file 
in the java directory.

6) To build the perl library see the README file in the perl directory.

BINARY INSTALL:
--------

Packaged binary releases of Spread are available from third parties
for several platforms, such as through the FreeBSD Ports system or as 
RPMs for Linux distributions. The Spread binary release consists of 
prebuilt binaries for several OS's that can be run without being installed
officially into the system.

We recommend that if you are experimenting with spread you create a special
'spread' directory (for example /usr/local/spread or /opt/spread) and keep
all the files together there so it is easy to find stuff.  That also makes 
it easier to run multiple architectures as the binaries for each are in their
own subdirectory. This is not necessary though. You can create that directory
anywhere (e.g. your own directory).

1) Unpack the spread-bin-4.x.tar.gz file into the target directory, 
like /opt/spread

2) Look at the Readme.txt for any updates

3) Look in the bin/{archname} directory for the appropriate architecture for
your system. For example, for Mac OS X 10.3 the binaries to run will be in 
bin/powerpc-apple-darwin7.9.0/

The libraries to link with your application will be found in the
/lib/{archname} directory

The include files to link with are in include/ and the documentation
can be found in docs/

4) To run spread you need to modify the 'docs/sample.spread.conf' file for
   your network configuration.  Comments in the sample file explain this.
   Then cp -p sample.spread.conf /usr/local/bin/spread.conf
   (Notice the name change.  The config file must be named that to be found.
    Alternatively you can run spread -c <config_file_name> )

    You can run "spread usage" to see the spread running options.

To use the Java classes and examples you need to have a copy of the 
main 'spread' daemon running.  Then the class files give you the 
equivelent of the libspread.a as a set of java classes.  The user.java,  
and user.class files give you a demonstration application and source code.  
Under the java/docs directory is a full verison of the Spread Java API
provided by javadocs. 

For Microsoft Windows systems use the spread.exe executable or spread-service.exe
service and the libspread.lib to link with your programs.

OVERVIEW:
---------

Spread consists of two parts, an executable daemon which is executed on 
each machine which is part of the heavyweight 'machine' group, and a library
which provides a programming interface for writing group multicast 
applications and which is linked into the application.

The daemon, called "spread", should be run as a non-priveledged 
user (we created a 'spread' user) and only needs permissions to create a 
socket in /tmp and read its config file which should be in the same 
directory as the executable.  By default the daemon binds to and runs 
off the non-priveledged port 4803 unless the config file indicates otherwise.

Each daemon can be started independently and if it does not find 
any other members of its defined configuration currently active it runs as 
a machine group of 1 machine.  When other daemons are started they will join
this machine group.

The machines which are running a daemon with a common config file 
form a 'machine group' (in contrast to a 'process group').  The daemons 
handle multicasting mesages between each other,providing ordering and 
delivery guarantees, detecting and handling failures of nodes or links, and 
managing all applications which are connected to each daemon.

Each application utilizing the spread system needs to link with 
the 'libspread.a', 'libtspread.a', 'libtspread.lib' or 'libspread.lib' 
library and needs to use the calls defined in the 'sp.h' and 'sp_func.h' 
header files. Documentation for the interface is currenly incomplete, but 
a basic application must do at least the following:

1) Before using any other spread calls you need to 'connect' to a daemon 
by calling the SP_connect(...) call.  This will set a mbox which is an integer 
representing your connection to the daemon.  You use this when making other
spread calls.  You CAN connect to daemons multiple times from the same 
application (and we know of times when this is very useful).

2) If you want to receive messages from a group you need to call SP_join().
Groups are named with standard alphanumeric strings.

3) To send to a group you do NOT need to join it, just call SP_multicast() 
with an appropriate group name.  You will not receive the messages back if 
you are not a member of the group.

4) To be nice to everyone else you should call SP_disconnect() when your 
program is finished using the spread system, if you do not your program's 
disconnection will eventually be detected. 

Some important observations when writing spread programs:

1) All the messages for a particular connection are received during an 
SP_receive() call.  This includes messages from different groups and 
membership messages(like group leave and join), so you must demultiplex 
them yourself.  This is a feature. You can opt to avoid getting membership 
messages all together by indicating that to SP_connect().

2) Spread handles endian differences correctly for metadata and our headers 
but does NOT convert your data for you.  It DOES tell you in the SP_receive 
call whether or not the data originated at machine with a different 
endianness then yours so you can easily use this to convert it yourself 
if necessary. 

3) Each connection to a daemon has a unique 'private group' name which 
can be used to send a message to a particular, and only to a particular, 
application connection.  

4) Spread supports the following ordering/delivery guarantees for messages:

	Unreliable	(least)
	Reliable	(will get there, no ordering)
	Fifo		(reliable and ordered fifo by source)
	Causal		(reliable and all mesg from any source of this level
			are causally ordered)
	Agreed		(reliable and all mesg from any source of this level 
			are totally ordered)
	Safe		(Agreed ordering and mesg will not be delivered to 
			application until the mesg has reached ALL 
			receipients' daemons)

For more detail, much of which IS important, see papers on 
Extended Virtual Sychrony, Transis and Totem.

UTILITY PROGRAMS:
-----------------

	Spread comes with several demonstration programs and useful tools.
They can be found in the main daemon directory as well as the example 
directory. 

1) spmonitor:
	This program has a special interface to spread which allows it to 
control a machine group.  It can terminate all the spread daemons in the 
group, change flow control parameters, and monitor the stats for one or all 
of the machines.  It needs to know the spread.conf file used for a particular
 set of spread daemons.

2) spuser:

	This program is provided with source and uses all the functions of 
the spread interface.  It also acts as a simple client which is useful for 
testing. Reading the source code to this program should show you how to use
all the features of spread, and answer many questions about syntax details.

3) simple_user:

	This is just about the simplest spread program possible and is 
provided with source.  It sends and receives text strings.

4) spflooder:
	
	This is used as one type of performace test.  It 'floods' a spread 
group with data messages. It is provided with source.

5) flush_user:
        This provides a demonstration of using the FL_ interface that provides
Virtual Synchrony semantics to your application. Just like the spuser program
it allows you to join groups and send and receive messages. 

CONTACT:
--------

If you have any questions please feel free to contact:

   spread@spread.org
~~~


## license:

~~~
Spread Open-Source License -- Version 1.0
-----------------------------------------
Copyright (c) 1993-2006 Spread Concepts LLC.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following request and
   disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following request and
   disclaimer in the documentation and/or other materials provided
   with the distribution.

3. All advertising materials (including web pages) mentioning features
   or use of this software, or software that uses this software, must
   display the following acknowledgment: "This product uses software
   developed by Spread Concepts LLC for use in the Spread toolkit. For
   more information about Spread see http://www.spread.org"

4. The names "Spread" or "Spread toolkit" must not be used to endorse
   or promote products derived from this software without prior
   written permission.

5. Redistributions of any form whatsoever must retain the following
   acknowledgment: "This product uses software developed by Spread
   Concepts LLC for use in the Spread toolkit. For more information about
   Spread, see http://www.spread.org"

6. This license shall be governed by and construed and enforced in
   accordance with the laws of the State of Maryland, without
   reference to its conflicts of law provisions. The exclusive
   jurisdiction and venue for all legal actions relating to this
   license shall be in courts of competent subject matter jurisdiction
   located in the State of Maryland.

TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, SPREAD IS PROVIDED
UNDER THIS LICENSE ON AN AS IS BASIS, WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
THAT SPREAD IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR
PURPOSE OR NON-INFRINGING. ALL WARRANTIES ARE DISCLAIMED AND THE
ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE CODE IS WITH
YOU. SHOULD ANY CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT THE
COPYRIGHT HOLDER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY
NECESSARY SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY
CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE. NO USE OF ANY CODE IS
AUTHORIZED HEREUNDER EXCEPT UNDER THIS DISCLAIMER.

TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR ANY OTHER CONTRIBUTOR BE LIABLE FOR ANY
SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES FOR LOSS OF
PROFITS, REVENUE, OR FOR LOSS OF INFORMATION OR ANY OTHER LOSS.

YOU EXPRESSLY AGREE TO FOREVER INDEMNIFY, DEFEND AND HOLD HARMLESS THE
COPYRIGHT HOLDERS AND CONTRIBUTORS OF SPREAD AGAINST ALL CLAIMS,
DEMANDS, SUITS OR OTHER ACTIONS ARISING DIRECTLY OR INDIRECTLY FROM
YOUR ACCEPTANCE AND USE OF SPREAD.

Although NOT REQUIRED, we at Spread Concepts would appreciate it if
active users of Spread put a link on their web site to Spread's web
site when possible. We also encourage users to let us know who they 
are, how they are using Spread, and any comments they have through 
either e-mail (spread@spread.org) or our web site at 
(http://www.spread.org/comments).

~~~
