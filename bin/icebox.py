#!/usr/bin/env python
####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov>
# Copyright (C) 2000 Regents of the University of California
# See ./DISCLAIMER
####################################################################

import sys
import commands
import string
import getopt
import os
import fcntl, FCNTL
import time
import signal
import pm_utils, pm_classes
import termios, TERMIOS
import random

class NodeDataClass:
    "Class definition for icebox attached nodes"
    type = ""
    tty  = ""
    box  = ""
    port = ""

    def __init__(self, type, vals):
        "Node class initialization"
        self.type = type
        try:
            self.tty  = vals[0]
            self.box  = vals[1]
            self.port = vals[2]
        except IndexError:
            pm_utils.exit_error(6, str(vals))
        # print self.name, self.tty, self.box, self.port

class PortClass:
    "Class definition for an icebox port"
    name       = ""
    node       = None
    com        = ""
    reply      = ""
    PORT_DELAY = 0.3
    
    def __init__(self, node):
        "Port class initialization"
        try:
            port_name = node.q_data.port
        except AttributeError:
            port_name = node.c_data.port
        self.name      = port_name
        self.node      = node
        self.com       = ""
        
    def do_command(self):
        # print "Doing port command", com
        box = self.node.c_data.box
        target = 'c' + box + self.com + self.name
        retry_count = 0
        good_response = 0
        while (not good_response and (retry_count < 10)):
            retry_count = retry_count + 1
            response = pm_utils.prompt(target)
            if (string.lower(response) == 'ok'):
                good_response = 1
        if (not good_response):
            pm_utils.exit_error(21, self.name + ",box " + box)
        time.sleep(self.PORT_DELAY)
            
class BoxClass:
    "Class definition for an icebox"
    name             = ""
    ports            = {}
    com              = ""
    ICEBOX_SIZE      = 10
    # Setting BOX_DELAY to 3.5 seconds results in always having exactly
    # one "ERROR" reply before success.
    BOX_DELAY        = 4.0

    def __init__(self, name):
        "Box class initialization"
        self.name  = name
        self.ports = {}
        self.com   = ""

    def add(self, node):
        "Add a port to the list of occupied ports on the icebox"
        port = PortClass(node)
        self.ports[port.name] = port

    def do_command(self):
        num_requested = 0
        req = ""
        for port_name in self.ports.keys():
            port = self.ports[port_name]
            if (port.node.is_marked()):
                if (not req): req = port.node.message
                num_requested = num_requested + 1
        if (not req): return
        setting = 0
        reverse = 0
        return_immediately = 0
        if (req == 'query'):
            com = 'ns'
        elif (req == 'rquery'):
            reverse = 1
            com = 'ns'
        elif (req == 'off'):
            setting = 1
            com = 'pl'
        elif (req == 'on'):
            setting = 1
            com = 'ph'
        elif (req == 'reset'):
            setting = 1
            com = 'rp'
        elif (req == 'temp'):
            com = 'ts'
        elif (req == 'tempf'):
            com = 'tsf'
        elif (req == 'hwreset'):
            return_immediately = 1
            com = 'rb'
        else:
            pm_utils.exit_error(3, req)
        if ((num_requested < self.ICEBOX_SIZE) and setting):
            for port_name in self.ports.keys():
                port = self.ports[port_name]
                if (port.node.is_marked()):
                    port.com = com
                    port.node.unmark()
                    port.do_command()
            return
        # print "Doing whole box command", com
        target = 'c' + self.name + com
        retry_count = 0
        good_response = 0
        while (not (good_response == 10) and (retry_count < 10)):
            retry_count = retry_count + 1
            response = pm_utils.prompt(target, return_immediately)
            if(response == ''):
                continue
            if (setting or return_immediately):
                if (string.lower(response) == 'ok'):
                    good_response = 10
                    for port_name in self.ports.keys():
                        port = self.ports[port_name]
                        port.com = ""
                        port.node.unmark()
                    self.com = ""
                    continue
            # now we're dealing with some sort of query
            try:
                list = string.split(response)
            except ValueError:
                continue
            for resp in list:
                try:
                    p,val = string.split(resp, ':')
                except ValueError:
                    continue
                if(p[0:1] != 'N'):
                    continue
                port_name = p[1:2]
                try:
                    port = self.ports[port_name]
                except KeyError:
                    continue
                node = port.node
                if(port.reply == ""):
                    # If a reply is already present then this is a second or
                    # later try to get all ten responses from the
                    # box, i.e. some of the response was garbled.
                    good_response = good_response + 1
                    if (com == 'ns'):
                        node.state = val
                        if (((node.state == '0') and reverse) or ((node.state == '1') and not reverse)):
                            if (node.is_marked()):
                                port.reply = node.name
                                node.unmark()
                            else:
                                port.reply = ""
                        else:
                            port.reply = ""
                    elif((com == 'ts') or (com == 'tsf')):
                        temps = val
                        port.reply = "%s:%s" % (node.name,temps)
                        node.unmark()
        if (good_response == 10):
            for port_name in self.ports.keys():
                port = self.ports[port_name]
                port.node.mark(port.reply)
        else:
            pm_utils.exit_error(21, "box " + self.name)
        if (setting):
            time.sleep(self.BOX_DELAY)

class TtyClass:
    "Class definition for a tty with icebox(es) attached"
    name       = ""
    boxes      = {}
    prev_attrs = None
    attrs      = None
    device     = None
    locked     = 0

    def __init__(self, name):
        "Tty class initialization"
        self.name       = name
        self.boxes      = {}
        self.attrs = None
        self.prev_attrs = None
        self.device     = None
        self.locked     = 0

    def do_command(self):
        # print "Doing tty command", com
        pm_utils.init_tty(self)
        for box_name in self.boxes.keys():
            box = self.boxes[box_name]
            box.do_command()
        pm_utils.fini_tty(self)
        
    def add(self, node):
        try:
            box_name = node.q_data.box
        except AttributeError:
            box_name = node.c_data.box
        try:
            box = self.boxes[box_name]
        except KeyError:
            box = BoxClass(box_name)
            self.boxes[box_name] = box
        box.add(node)


class SetDataClass:
    "Structure in which to gather all the icebox info in a single place"
    ttys    = {}
    cluster = None

    def __init__(self, cluster):
        "Read in the node, tty, box, and port for each node from the configuration file"
        self.ttys    = {}
        self.cluster = cluster
        
    def add(self, node):
        try:
            tty = self.ttys[node.q_data.tty]
        except KeyError:
            tty = TtyClass(node.q_data.tty)
            self.ttys[tty.name] = tty
        tty.add(node)

    def do_com(self):
        "carry out the requested command on each tty line"
        pm_utils.init_alarm()
        for tty_name in self.ttys.keys():
            tty = self.ttys[tty_name]
            tty.do_command()

