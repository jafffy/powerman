/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton (uselton2@llnl.gov>
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

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "powerman.h"
#include "list.h"
#include "wrappers.h"
#include "hostlist.h"
#include "hash.h"
#include "arglist.h"

struct arglist_iterator {
    hostlist_iterator_t itr;
    ArgList arglist;
};

struct arglist {
    hash_t args;                /* hash of Arg structs */
    hostlist_t hl;              /* list of node name keys (to preserve order) */
    int refcount;               /* free when refcount == 0 */
};

/* destroy an Arg */
static void _destroy_arg(Arg * arg)
{
    assert(arg != NULL);
    assert(arg->node != NULL);
    Free(arg->node);
    if (arg->val)
        Free(arg->val);
    Free(arg);
}

/* create an ArgList */
ArgList arglist_create(hostlist_t hl)
{
    ArgList new = (ArgList) Malloc(sizeof(struct arglist));
    hostlist_iterator_t itr;
    Arg *arg;
    char *node;
    int hash_size;

    new->refcount = 1;
    hash_size = hostlist_count(hl); /* reasonable? */
    new->args = hash_create(hash_size, (hash_key_f)hash_key_string, 
            (hash_cmp_f)strcmp, (hash_del_f)_destroy_arg);

    if ((itr = hostlist_iterator_create(hl)) == NULL) {
        arglist_unlink(new);
        return NULL;
    }
    while ((node = hostlist_next(itr)) != NULL) {
        arg = (Arg *) Malloc(sizeof(Arg));
        arg->node = Strdup(node);
        arg->state = ST_UNKNOWN;
        arg->val = NULL;
        hash_insert(new->args, arg->node, arg);
    }
    hostlist_iterator_destroy(itr);
    new->hl = hostlist_copy(hl);

    return new;
}

/* decrement the refcount for an ArgList and free when zero */
void arglist_unlink(ArgList arglist)
{
    if (--arglist->refcount == 0) {
        hash_destroy(arglist->args);
        hostlist_destroy(arglist->hl);
        Free(arglist);
    }
}

/* increment the refcount for an ArgList */
ArgList arglist_link(ArgList arglist)
{
    arglist->refcount++;
    return arglist;
}

bool arglist_get(ArgList arglist, char *node, char *val, InterpState *state)
{
    Arg *arg = NULL;

    if (node != NULL)
        arg = hash_find(arglist->args, node);
    if (arg) {
        if (val)
            val = arg->val;
        if (state)
            *state = arg->state;
    }

    return arg ? TRUE : FALSE;
}

void arglist_set(ArgList arglist, char *node, char *val, InterpState state)
{
    Arg *arg;

    if (node != NULL && (arg = hash_find(arglist->args, node)) != NULL) {
        arg->val = Strdup(val);
        arg->state = state;
    }
}

ArgListIterator arglist_iterator_create(ArgList arglist)
{
    ArgListIterator itr = (ArgListIterator)Malloc(sizeof(struct arglist_iterator));

    itr->arglist = arglist;
    itr->itr = hostlist_iterator_create(arglist->hl);

    return itr;
}

void arglist_iterator_destroy(ArgListIterator itr)
{
    hostlist_iterator_destroy(itr->itr);
    Free(itr);
}

Arg *arglist_next(ArgListIterator itr)
{
    Arg *arg = NULL;
    char *node;

    node = hostlist_next(itr->itr);
    if (node != NULL)
        arg = hash_find(itr->arglist->args, node);

    return arg;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */