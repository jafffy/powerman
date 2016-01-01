Quick information on configuring various terminal servers is here.

## Cyclades TS-1000 ##

To configure the Cyclades TS-1000  so  that  PowerMan  can  connect  to
serial-port-based remote power control devices using telnet.
Attach console cable between laptop running minicom and console port at
`9600,8n1` and authenticate with default account: `root/tslinux`.

Edit the following files (assumes no off-subnet routing or name service required):\

`/etc/portslave/portslave.conf`
```
conf.eth_ip      192.168.54.151
conf.eth_mask    255.255.255.0
conf.dhcp_client 0
#
all.speed        9600
all.datasize     8
all.stopbits     1
all.parity       none
all.flow         none
all.dcd          0
all.DTR_reset    1
all.protocol     socket_server
all.authtype     none
#all.ipno
all.socket_port  7001+
#
s1.tty           ttyS1
s2.tty           ttyS2
s3.tty           ttyS3
s4.tty           ttyS4
s5.tty           ttyS5
s6.tty           ttyS6
s7.tty           ttyS7
s8.tty           ttyS8
s9.tty           ttyS9
s10.tty          ttyS10
s11.tty          ttyS11
s12.tty          ttyS12
s13.tty          ttyS13
s14.tty          ttyS14
s15.tty          ttyS15
s16.tty          ttyS16
```

`/etc/hostname`
```
cyclades-ts-test
```

`/etc/hosts`
```
127.0.0.1        localhost
192.168.54.151   cycaldes-ts-test
```

Next, run `signal_ras hup`  and `saveconf` and finally, power cycle the
unit.  It should be possible to configure PowerMan to connect to
`hostname:7001` for port 1, `hostname:7002` for port 2, etc..

## MRV In-Reach IR-8020 ##

To  configure  the  In-Reach (formerly Xyplex) IR-8020 so that PowerMan
can connect to serial-port-based remote  power  control  devices  using
telnet.

To  reset  configuration:  attach terminal to any port, press button on
front with paper clip until all LED’s  light  up;  release,  then  hold
again  for  3-5  seconds until sweeping pattern stops; led’s will count
down, then press return on terminal several times.  After
`Configuration in Progress` message, type `access`.  Then select
`2. Modify unit configuration`, then
`3. Initialize server and port parameters`, then
`D`  to  use defaults, `S`  to save them, `X` to go back to the main menu,
and  to exit saving config.  The terminal server will now reboot and obtain
its network config from BOOTP.

Telnet to port 2000 and at the `Login>` prompt, type `access`, and anything
you like at the `Enter username>` prompt.  At the `In-Reach>` prompt,  type
`set priv`,  then `default password system`.  You should now have the
`In-Reach_Priv>` prompt.  Now enter the following:

```
define server change enabled
define login password "access"
define priv password "system"
define server parameter server check disabled
define po 1-16 autoconnect enabled
define po 1-16 inactivity logout disabled
define po 1-16 flow control disabled
define po 1-16 autobaud disabled
define po 1-16 autoprompt disabled
define po 1-16 message codes disabled
define po 1-16 internet connect disabled
define po 1-16 broadcast disabled
define po 1-16 input flow control disabled
define po 1-16 output flow control disabled
define po 1-16 loss notification disabled
define po 1-16 line editor disabled
define po 1-16 verification disabled
define po 1-16 access remote
define po 1-16 break disabled
define po 1-16 tel newline nul
define po 1-16 tel binary session mode pasthru
define po 1-16 default sessions mode pasthru
define po 1-16 tel csi escape disabled
define po 1-16 type ansi
define po 1-16 speed 115200
define po 1-16 char size 8
define po 1-16 parity none
define po 1-16 typeahead size 2048
define po 1-16 ip tcp window size 2048
```

To reboot, type `init del 1`.

It should be  possible  to  configure  PowerMan  to  connect  to
`hostname:2100` for port 1, `hostname:2200` for port 2, etc..
