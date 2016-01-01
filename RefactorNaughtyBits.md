#summary Refactoring Powerman Device Core [D R A F T ](.md)

### Introduction ###

The most complex and hard to maintain part of powerman is its device core.
This a big poll() loop that executes the powerman scripting language,
advancing state as data is transmitted and received on sockets, ptys,
or serial ports connected to remote power controllers.

Powerman has the capability to spawn _coprocesses_ and communicate with
them bidirectionally on ptys.  The coprocesses are driven just like a remote
power controller, using the powerman scripting language.  Coprocesses are
the basis for the powerman test suite, where simple emulators for supported
power control devices are exercised.  It has also enabled powerman to
accrue new connectivity modes such as SNMP and IPMI that did not
map well onto the poll-based architecture of the device core.

This document describes a structural change to the powerman code that
will push the powerman scripting language out of the device core into
a coprocess instance.  Coprocesses (now called _agents_) will be spawned
as tightly coupled threads and driven directly by the device core
through a new messaging layer, rather than through the scripting language.

These changes should improve code maintainability and allow powerman
to scale to a greater number of devices.  Simplifying the device core
and scripting engine should allow a number of long time oustanding issues
to be addressed.  Finally, the road is paved for a fully distributed
powerman architecture which could become fault tolerant and scalable
to an even larger number of devices.

### 0MQ Messaging ###

The [0MQ](http://www.zeromq.org/) messaging framework is the basis for
the device core to communicate with agent threads.  0MQ bills itself
as "sockets on steroids".  For an entertaining introduction, read
[0MQ - The Guide](http://zguide.zeromq.org/page:all).  0MQ can be used
as a threaded concurrency framework for messaged agents.
See the blog post
[Multithreading Magic](http://www.zeromq.org/blog:multithreading-magic)
which argues messaged agents over traditional threaded synchronization
mechanisms.

In the proposed rework, powerman would use the 0MQ _inproc_ (shared memory)
socket type.  The synchronous request-reply messaging pattern would be
used between device core (REQ) and agents (REP).  Each device instance
would have its own socket, inproc://_devname_.

![http://wiki.powerman.googlecode.com/git/images/agent_small.png](http://wiki.powerman.googlecode.com/git/images/agent_small.png)
_### Google Protocol Buffers ###_

Since agents are driven by request/response messages rather than a
command line interface emulating a remote power controller, a scheme
needs to be chosen to encode message data.  An implementation of Google
[Protocol Buffers](http://code.google.com/p/protobuf/)
for C called
[protobuf-c](http://code.google.com/p/protobuf-c/)
is used to generate encode/decode functions from a high level
description messages.

This approach should minimize the number of new bugs introduced
in this area, make the protocol easier to change going forward,
and enable others to work on the project with less of a learning curve.

### Client Protocol ###

The client currently uses a plain text protocol to communicate with the
powerman server.  This could also be converted to 0MQ/protobuf, for example
using the REQ-ROUTER message pattern on a tcp socket type.
The ROUTER pattern allows the server to take requests from multiple
clients on the same socket, interleaving requests and responses,
and getting replies back to the right client.

![http://wiki.powerman.googlecode.com/git/images/client_small.png](http://wiki.powerman.googlecode.com/git/images/client_small.png)

Updating the client protocol is not strictly required to meet the
primary goal of this rework, which is simplifying the device core,
but it does make sense to use the same approach throughout the code base.

The client to server communication lacks security now:

  * [issue 27](https://code.google.com/p/powerman/issues/detail?id=27) powerman client should authenticate to server

This should be remedied in a new protocol.  Details TBD.

### Powerman Scripting Agent ###

Although it is tempting to replace the powerman scripting language using
an off-the-shelf embeddable language like [lua](http://www.lua.org/)
(see [issue 19](https://code.google.com/p/powerman/issues/detail?id=19)), there is now a body of tested scripts in the powerman
distribution for various power controllers, not all of which are easy
to dredge up for re-testing.  The powerman scripting engine needs to
be maintained for the foreseeable future, but it doesn't need to play
such a central role in powermand.  In the new scheme, it is migrated
to an agent.

One instance of the powerman scripting agent is created for each
power controller _device_ instance.  Each agent runs in its own
thread and is implemented using the strictly synchronous REP messaging
pattern (one request/response at a time).

This should greatly simplify the scripting engine implementation
since it no longer has to save/restore state to switch betweeen
concurrently executing devices as asynchronous I/O events occur,
and it is no longer co-mingled with client request processing, etc..

This simplification should enable various bugs to be addressed, e.g.

  * [issue 10](https://code.google.com/p/powerman/issues/detail?id=10): need power (watts) monitoring capability
  * [issue 12](https://code.google.com/p/powerman/issues/detail?id=12): interpret and normalize temperature output
  * [issue 13](https://code.google.com/p/powerman/issues/detail?id=13): problem with reconnect logic
  * [issue 14](https://code.google.com/p/powerman/issues/detail?id=14): allow %s to appear multiple times
  * [issue 15](https://code.google.com/p/powerman/issues/detail?id=15): device timeout should be per expect line
  * [issue 16](https://code.google.com/p/powerman/issues/detail?id=16): generate error if %s appears in script where it is undefined
  * [issue 17](https://code.google.com/p/powerman/issues/detail?id=17): command successful reported unconditionally
  * [issue 18](https://code.google.com/p/powerman/issues/detail?id=18): clean up arg passing in and out of scriptlets
  * [issue 20](https://code.google.com/p/powerman/issues/detail?id=20): logoff script is unused
  * [issue 21](https://code.google.com/p/powerman/issues/detail?id=21): add option to drop connection to RPC between commands
  * [issue 24](https://code.google.com/p/powerman/issues/detail?id=24): add mechanism for querying RPC's directly
  * [issue 25](https://code.google.com/p/powerman/issues/detail?id=25): powerman could try harder to validate RPC config
  * [issue 26](https://code.google.com/p/powerman/issues/detail?id=26): allow arbitrary scripts (device specific) to be executed

All of these issues require changes to the device core, and all
of them have languished on powerman's todo list for years because of
the effort required to change anything in that part of the code.

One point on the synchronous REP pattern for agents:
This means only one command per power controller can be
outstanding at a time.  This is the case in powerman now, and although
it may seem like a confining limitation going forward, in reality,
most operations complete quickly and one could argue that
it is unwise to try to push embedded firmware too hard by issuing
more than one command at a time.  The code simplification achieved
through synchronous programming is worth perpetuating this limitation.

The one command that we have now which does take a long time is _cycle_.
It turns out there are other reasons that _cycle_ should be broken up
into _off_, _delay_, _on_ at the agent level:

  * [issue 9](https://code.google.com/p/powerman/issues/detail?id=9): cycle command does not work right for aliased redundant supplies on same RPC

Therefore, this will be the plan in the new code.

### Client Requests ###

As clients make high level requests, like "turn on these 10 nodes",
these high level requests are broken down into _actions_ that are
enqueued on each device.  Although device actions are serialized,
multiple device queues are run in parallel.  For example, a client
asks to turn on nodes t1 through t10 which happen to
span two plugs on five devices.  The operation can be performed five
plugs at a time; that is, five device queues will each have two actions
in their queue, and the queues are managed in parallel.

This does not change in the proposed restructuring.
There is still a queue of actions per device, and the device core's
primary responsibility is to keep the agents for each device instance
fed with requsts from that device's queue.  A poll() loop still needs
to exist in this code, but zmq\_poll() is used, and events will be
whole messages from agents rather than ready file descriptors.
(N.B. client file descriptors will have to be polled via zmq\_poll()
if the client protocol is not moved to 0MQ).

### Config File ###

The `powerman.conf` file consists of configuration directives written
in the powerman
scripting language, and _include_ directives to pull in needed
device scripts.  In the new plan, each instance of the powerman scripting
agent could be pointed to a device script file, and the higher level
config file could be decoupled and possibly reworked to address some of
its limitations, like the inability to associate more than one plug with
a node for redundant power supplies, except using aliases.

Lua is an excellent candidate for the higher level config file, as described in
[Programming in LUA:  Chapter 25 - Extending your Application](http://www.lua.org/pil/25.html).

Details TBD

### Legacy Coprocesses ###

Although the coprocesses shipped with powerman like
snmppower, httppower, and plmpower would be converted
to agents, the ability to drive coprocesses via the powerman scripting
langage agent must be retained to support the powerman test suite,
and externally provided coprocesses like ipmipower and hp3488.

It should be noted though that there may be benefits to migrating
some externally provided coprocesses like ipmipower to native powerman
agents.  For example, error handling could be more sophisticated,
and scalability could be improved.

### Distributed Powerman ###

Agents could be distributed if the 0MQ socket type is changed from
inproc to, say, tcp.  An external service could start the agents
somewhere on the network, keep them running as nodes go up and down,
and provide location brokerage service to powermand.

Or multiple powermands could be federated as shown below, such that each
is responsibile for a subset of the total number of agents.
A client could connect to any powermand, and it could proxy requests
to other powermands as needed.  This improves scalability over a single
powermand architecture by reducing the number of agent threads and
poll event sources any one powermand has to deal with.  Powermands
could take over the agents of a failed powermand to implement
fault tolerance.

![http://wiki.powerman.googlecode.com/git/images/dist_small.png](http://wiki.powerman.googlecode.com/git/images/dist_small.png)

0MQ enables distribution of powerman components in a number of different
ways, should that need to be pursued.
The details are beyond the scope of this document.