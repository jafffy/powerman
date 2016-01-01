## Introduction ##

powerman  device  specifications are rather wierd, and the intent is to
replace them with embedded Lua at some point.  For this reason, we
suggest that you leave the writing of these  scripts  to  the  powerman
authors.  However, if you insist, here is how they work.

Note: the authors do not guarantee that the powerman specification lan
guage will not change, however we are open to taking on maintenance  of
scripts	submitted by powerman users.  We can't guarantee that we'll be
able to test new releases against all devices but we'll do our best not
to  break  anything.  NOTE: the best way to help us in this endeavor is
to provide a ``simulator'' for your  power  controller  and  associated
tests  in  the  test subdirectory of the powerman source code.  See the
examples in that directory.

## Language ##

By convention, device scripts are one device per file and are  included
as needed from a powerman.conf file, like this:

```
   include "/etc/powerman/icebox3.dev"
```

A device script is surrounded by an outer block:
```
   specification "my_device_name" {
      # configuration settings
      # script blocks
   }
```

### Configuration Settings ###

The possible configuration settings are:

**timeout** _float_ : (optional)  device  script  timeout in seconds - applies
to each script, the whole thing, not just a particular **expect**.

**plug name** { _string list_ } : (optional) if plug names are static,
they  should  be  defined.  Any reference to a plug name in the
powerman.conf must match one of the defined plug names.

**pingperiod** _float_ : (optional) if a  ping  script  is  defined,
and	pingperiod  is nonzero,	the  ping  script will be executed
periodically, every _float_ seconds.

### Scripts ###

Script blocks have the form:
```
   script <script_name> {
      # statements
   }
```

Script blocks should all be grouped together with no  config  lines  in
between.   Scripts  are	for  performing  particular operations such as
power on, get power status, etc.  The various script names  are	listed
below.	Those marked with (%s) are called with a plug name _argument_,
which can be included in a send statements by including	a  %s  (printf
style).	 Warning:  all	the send strings are processed with printf and
you can cause powermand to segfault if you include  any	printf	tokens
other than the appropriate zero or one %s.

**login** : Executed immediately  on (re-)connect.  If you need to login to
the box, do it here.  This is  also  a  good  place  to  descend
through  a  first  layer of menus.  Caveat: % occurring in pass‐
words must be escaped as %%.
Caveat: occurs outside  of client session  so  cannot be debugged
with -T. A trick when debugging is to move this code into the status
script temporarily  so  you can see what is going on.

**logout** :  Executed	prior  to  disconnect.	Get device in a state so login
script will work (though hopefully disconnecting	will  do  that too).

**status\_all**, **status(%s)** : Obtain  plug  state  for	all  plugs or
only the specified plug.  When all plugs of a device are involved in a
plug status	query, the  status\_all script, if defined, will be called
in preference to the status script; otherwise the status script is called
for each plug.

**on\_all**, **on\_range(%s)**, **on(%s)** : Power on all plugs, a range of plugs,
or the specified plug.

**off\_all**, **off\_range(%s)**, **off(%s)** : Power off all plugs, a range of
plugs, or the specified plug.

**cycle\_all**, **cycle\_range(%s)**, **cycle(%s)** : Power  cycle all plugs, a
range of plugs, or the specified plug.  The intent of this command was
to map to the  RPC's  cycle  command;  however, device script are increasingly
implementing this in terms of a power off/delay/power so the off time can
be  controlled by the script.

**status\_temp\_all**, **status\_temp(%s)** : Obtain  temperature  reading for
all plugs or only the specified plug.  Temperature is obtained by sampling
a thermocouple in the node.   Results  are reported as a text string - not
interpreted by Powerman beyond any regex chopping done by the script.

**status\_beacon\_all**, **status\_beacon(%s)** : Obtain beacon state for all
plugs or only  the  specified  plug.  Some RPC's include a way to flash
a light on a node.

**beacon\_on(%s)** : Flash beacon on the specified plug.

**beacon\_off(%s)** : Clear beacon on the specified plug.

**reset\_all**, **reset\_range(%s)**, **reset(%s)** : Reset  all  plugs, a range
of plugs, or only the specified plug.  Reset refers to signaling a
motherboard reset butten header, not a plug cycle.

### Statements ###

Within a script, the following statements can be used:

**send** _string_ : Send _string_ to the device.

**delay** _float_ : Pause script for _float_ seconds.

**expect** _string_ : _string_	is  compiled  as a regular expression with
regcomp(3).  The regular expression is matched  against  device  input.   The
script  blocks  until the regex is matched or the device timeout
occurs (in which case the script is  aborted).   Upon  matching,
any parenthesized expressiones are assigned to variables: $1 for
the first match, $2 for the second match, and so	on.
Warning: some  implementations  of  regex(3) silently fail if the regular
expression exceeds available static storage.

**setplugstate** (_string_|_regmatch_) _regmatch_	(_off_=_string_) (on=_string_) : Set the plug state.  The first argument, if present, is the literal plug
name or a _regmatch_ from the  previous  expect  which contains	the  plug
name.  If omitted, the plug name is presumed to be the script argument.
The off and on strings are  compiled regexes,	which if matched by
the second argument, result in the plug state being set to off or on.
Yes we are applying  regexes to regmatches!  If no off or on strings are
provided, state will be unknown.

**ifoff**, **ifon** : Script statements enclosed in an ifon/ifoff  block
are conditional  executed based on the state of the plug passed in as an
argument.  Ifon/ifoff blocks can only be	used  in  single  plug
scripts that take an argument.

**foreachplug** : Script  statements  enclosed in a foreachplug block are
executed iteratively with a %s argument defined  for  each  target  plug.
Foreachplug  blocks  can	only  be used in all plug scripts that
take no argument.

### Terminals ###

Script terminals are  defined as follows:

_float_ : decimal number - exponent forms not supported

_string_ : Text surrounded by double quotes.  May  contain  C  style  back‐
slash-escaped  characters,  including  three digit octal values,
and most common backslash-escaped single character values.

_string list_ : Multiple _string_ values separated by white space.

_script\_name_ : Name of script (see above).

_regmatch_ : Results of a parenthesized regular expression match are assigned
to $1, $2, ... $N.