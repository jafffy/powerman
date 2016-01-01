Quick information on configuring various devices is shown here.

## Baytech RPC-3 (non-NC models) (network port 23) ##

Attach console cable between laptop running minicom and console port at
9600,8n1.  Main menu should display.

From main menu, select `3)...Configuration`, then set IP parameters.

From  Configuration menu, select `5)...Access`, then disable command
confirmation, set  admin  password  to  `baytech`,  and  enable  prompt  for
password.  Exit Access menu by typing return.

From  Configuration menu, select `6)...Outlets`.  Ensure that
`1)...Outlet Status display` is disabled, and `2)...Command Confirmation`
is  disabled.

Exit  back  to  main  menu by typing return twice, then answer `Y` to the
`Accept changes?` prompt.

## Baytech RPC-28-NC (serial 9600,8n1) ##

Attach console cable between laptop running minicom and console port at
9600,8n1.   Type return to get `RPC-28>` prompt.  Type Help for a list of
commands.

Select config.  Select  `3)...Enable/Disable Confirmation`  and  disable
confirmation.   Select `4)...Enable/Disable Status Menu` and disable status
menu.

For network, type ;;;;; to access network configuration menu.

## LNXI Icebox V2 (network port 1010) ##

Attach console cable between laptop running minicom and console port at
9600,8n1.  Enter default password of `icebox`.

`net ip 192.168.54.153` sets IP address.

`net mask 255.255.255.0` sets network mask.

`net gw 192.168.54.254` sets default  gateway.
Note:  this  may  need  to be set to an address in the same subnet
as the ice box regardless of  whether it is to be used or not.

`reboot` Reboots the icebox.

## LNXI ICEBOX V3 (network port 1010) ##

Attach console cable between laptop running minicom and console port at
9600,8n1.  Authenticate with default account admin/icebox.   Configuration
is the same as the V2  Icebox.

## APC Masterswitch (network port 23) ##

Attach  console  cable  between laptop running minicom and console port
(may be labeled ‘‘advanced port’’)  at  2400,8n1.   Press  enter,  then
authenticate with default account `apc` password `apc`.

Choose `Network` from main menu.

Choose `TCP/IP` from Network menu, then diable BOOTP, and accept changes.
Set the IP address,  subnet  mask,  and  default  gateway,  and  accept changes.

Pres ESC until the main menu appears, then select `Logout`.

## WTI RPS10 (serial 9600,8n1) ##

Set  address of box using the rotary dial on the back.  The Master
module must be set to address 0.  Slave modules may be set to unique
values  from  1-9.  The address corresponds to the plug name in the
`powerman.conf` file.

Setup switches on each module should be set to 9600 baud (sw1 down),
5 sec toggle delay (sw2 down), power up to previous state (sw3 down).

## WTI NPS (network port 23) ##

Attach console cable between laptop running minicom and console port at
9600,8n1.  Try password `wti` if prompted.

Select `/N - View/set network parameters`,  then  set  the  IP  address,
subnet mask, and gateway.

Select  `/G  -  General  paramters`,  then  disable command confirmation,
enable command echo, and set disconnect timeout to  the  maximum  value
(30 minutes).

Set password to `wti`.

## Cyclades PM8 and PM8i (serial 9600,8n1) ##

Attach  console  cable  between  laptop running minicom and serial port
9600,8n1.   Press  enter,  then  authenticate  with   default   account
`admin` password `pm8`.

Run `factory_defaults` command.

The PM8 can run in a mode where it can be daisy chained from a Cyclades
terminal server.  This mode is not yet supported by PowerMan.
