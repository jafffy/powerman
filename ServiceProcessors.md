Quick information on configuring various devices is shown here.

## Sun ILOM ##

The dedicated ILOM serial port (SER MGT RJ-45) runs at 9600,8n1 and the
default  login is root/changeme.  Use a Sun/NETRA/Cisco RJ45 adapter to
connect to Cyclades TS.

The dedicated ILOM ethernet port (NET MGT RJ-45) can be configured  for
DHCP  (the  default) or static IP settings.  To configure static IP via
the ILOM serial port, run:

```
cd /SP/network
set pendingipdiscovery=static
set pendingipaddress=xxx.xxx.xxx.xxx
set pendingipnetmask=xxx.xxx.xxx.xxx
set pendingipgateway=xxx.xxx.xxx.xxx
set commitpending=true
```

The ILOM network configuration can alternatively be set up via the
system  BIOS setup screen.  Hit F2 during boot to access BIOS setup, go to
the `Advanced` tab, select `IPMI 2.0 configuration` and
`set  LAN  configuration`.  Make your changes and commit them.

## IPMI (via Ipmipower) ##

IPMI  based power control is supported via FreeIPMI’s ipmipower.  It is
configured by running ipmipower in coprocess mode in `powerman.conf`.

Due to semantic differences between IPMI and traditional  remote  power
control  devices,  some  power  control operations may not seem to work
properly by default with Powerman.  For  example,  several  IPMI  power
control  operations  are allowed to return prior to the operation fully
completing.  A machine that has been powered off by IPMI may  be  later
queried  as  being  powered  on.  This is because IPMI may successfully
return from a power off operation to the  user,  but  the  machine  may
elect to power itself off at a later time.  In order to get around several
of these  issues,  it  is  recommended  that  the  user  configure
ipmipower  with the `--wait-until-on` and `--wait-until-off options`.  With
these options set, ipmipower will behave more like  traditional  remote
power control devices.

In  order  to  hide IPMI usernames and passwords from ps(1), it is also
recommended that the user configure the username and password  (and  at
your  discretion,  the  above options) using the FreeIPMI configuration
file and not in the powerman.conf file.  Please see the ipmipower  section
of the freeipmi.conf(5) manpage for more information.

Because  IPMI  usernames  and  passwords  are  sensitive,  the  default
FreeIPMI configuration file is only  readable  and  writable  by  root.
This may conflict with the default powermand daemon settings, thus mak-
ing the configuration file non-readable.  Administrators  may  wish  to
run  the powermand daemon as root or adjust the configuration file per-
missions as needed.

IPMI based beacon support is  also  supported  since  ipmipower  0.7.9.
Users should be aware that not all motherboards support physical system
identification through an LED.

## HP iLO ##

The `hpilo` script supports the Integrated Lights-Out  management  processor
in  HP  ProLiant  servers,  including both rack-mount and blade
servers.  Configure the iLO to allow Telnet access on port 23, and  add
a user `Admin` with password `Admin`.  You may have to reduce the minimum
password length on the Administration/Access/Options tab of the web
interface.

## HP MP ##

The `hpmp` script supports the Management Processor in HP non-cellular,
rack-mount Integrity servers.  Configure the MP to allow Telnet  access
with  the  `CM:SA`  command,  and make sure there’s a user `Admin` with
password `Admin`.  If the MP enforces a minimum  password  length,  you
may  have  to  use `uc -all default` to restore the default `Admin` / `Admin`
user.

## HP MP (cellular) ##

The `hpmpcell` script supports the Management Processor in HP mid-range
cellular  servers.   Configure  the  MP to allow Telnet access with the
`CM:SA` command, and make sure there’s a  user  `Admin`  with  password
`Admin`.

## HP MP (Superdome) ##

The `hpmpdome` script supports the Management Processor in HP Superdome
servers.  Configure the MP to allow Telnet access with the `CM:SA` command,
and make sure there’s a user `Admin` with password `Admin`.

## HP MP (Integrity blades) ##

The   `hpmpblade`  script  supports  the  Management  Processor  in  HP
Integrity blade servers.  Configure the MP to allow Telnet access  with
the `CM:SA` command, and make sure there’s a user `Admin` with password
`Admin`.