/*****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *  
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see <http://www.llnl.gov/linux/powerman/>.
 *  
 *  PowerMan is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  PowerMan is distributed in the hope that it will be useful, but WITHOUT 
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with PowerMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#ifndef PM_XPOLL_H
#define PM_XPOLL_H

typedef struct xpollfd *xpollfd_t;

int         xpoll(xpollfd_t pfd, struct timeval *timeout);
xpollfd_t   xpollfd_create(void);
void        xpollfd_destroy(xpollfd_t pfd);
void        xpollfd_zero(xpollfd_t pfd);
void        xpollfd_set(xpollfd_t pfd, int fd, short events);
short       xpollfd_revents(xpollfd_t pfd, int fd);
char       *xpollfd_str(xpollfd_t pfd, char *str, int len);

#define XPOLLIN      1
#define XPOLLOUT     2
#define XPOLLHUP     4
#define XPOLLERR     8
#define XPOLLNVAL    16

#endif /* PM_XPOLL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
