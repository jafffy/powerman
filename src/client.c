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

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "powerman.h"
#include "list.h"
#include "config.h"
#include "powermand.h"
#include "action.h"
#include "error.h"
#include "wrappers.h"
#include "string.h"
#include "buffer.h"
#include "client.h"
#include "util.h"

/* prototypes */
static Action *_process_input(Protocol * client_prot, Cluster * cluster,
			     Client * c);
static Action *_find_cli_script(Protocol * p, Cluster * cluster,
				  String expect);
static bool _match_cli_template(Cluster * cluster, Action * act,
				  regex_t * re, String expect);
/*
 *   Select has indicated that there is material read to
 * be read on the fd associated with the Client c. 
 *   If there was in fact stuff to read then that stuff is put
 * in the "from" buffer for the client.  If a (or several) 
 * recognizable command(s) is (are) present then it is interpreted 
 * and put in a Server Action struct which is queued to
 * be processed when the server is Quiescent.  If the 
 * material is recognizably wrong then an error is returned 
 * to the client.  If there isn't a complete command (no '\n')
 * then the "from" buf keeps the data for later completion.
 * If there was nothing to read then it may be time to 
 * close the connection to the client.   
 */
void cli_handle_read(Protocol * client_prot, Cluster * cluster,
		   List acts, Client * c)
{
    int n;
    Action *act = NULL;

    CHECK_MAGIC(c);

    n = buf_read(c->from);
    if ((n < 0) && (errno == EWOULDBLOCK))
	return;
    if (n <= 0) {
	/* EOF close or wait for writes to finish */
	c->read_status = CLI_DONE;
	return;
    }

    do {
	/* 
         *   If there isn't a full command left in the from buffer then this 
         * returns NULL. Otherwise it will return a newly created Action
         * act, extract the next command from the buffer, set the
         * Action fields client, seq, com, num and target for later
         * processing, and enqueue the Action.  
         */
	act = _process_input(client_prot, cluster, c);
	if (act != NULL)
	    list_append(acts, act);
    }
    while (act != NULL);

}


/*
 *   Some new stuff is in the "from" buf.  
 *   Look the suff over and try to build a Server Action.  
 * Returning a NULL means something is there but not enough 
 * to interpret.  If it's recognizably a bad command then
 * return a Server Action with com = PM_ERROR.  That and the
 * "PM_CHECK_LOGIN" command is responded to immediately.  The
 * others require sending the Server Action to the devices 
 * first.
 */
static Action *_process_input(Protocol * client_prot, 
		Cluster * cluster, Client * c)
{
    Action *act;
    String expect;

    /* Using NULL means I'll just get the next '\n' terminated string */
    /* but with any '\r' or '\n' stripped off */
    expect = util_bufgetline(c->from);
    if (expect == NULL)
	return NULL;

    act = _find_cli_script(client_prot, cluster, expect);
    str_destroy(expect);
    if (act != NULL) {
	act->client = c;
	act->seq = c->seq++;
    }
    return act;
}

/*
 *   Select has notified that it is willing to accept some 
 * write data.
 *   If the client is wanting to close the connection 
 * and the write buffer is clear then close the 
 * connection.
 */
void cli_handle_write(Client * c)
{
    int n;

    CHECK_MAGIC(c);

    n = buf_write(c->to);
    if (n < 0)
	return;			/* EWOULDBLOCK */

    if (buf_isempty(c->to))
	c->write_status = CLI_IDLE;
}

/*
 *   Returning a NULL means there is not a complete unprocessed
 * command in the "from" buffer.  I.e. nothing to be done.
 * A complete command would be indicated by a '\n'.  There
 * could be more than one.
 */
static Action *_find_cli_script(Protocol * p, Cluster * cluster, String expect)
{
    Action *act;
    bool found_it = FALSE;
    regex_t *re;

    act = act_create(PM_ERROR);
    /* look through the array for a match */
    while ((!found_it) && (act->com < (NUM_SCRIPTS - 1))) {
	act->com++;
	assert(p->scripts[act->com] != NULL);
	act->itr = list_iterator_create(p->scripts[act->com]);
	act->cur = list_next(act->itr);
	assert(act->cur->type == EXPECT);
	re = &(act->cur->s_or_e.expect.exp);
	found_it = _match_cli_template(cluster, act, re, expect);
	list_iterator_destroy(act->itr);
	act->itr = NULL;
    }
    if (!found_it)
	act->com = PM_ERROR;
    return act;
}

/*
 *   This function tries to match the given RegEx if it does then
 * depending on which command we're talking about it either returns
 * TRUE and is doen, or it goes on to scan for an RegEx that the
 * Client sent in.  Finding one, it puts it in tthe Action's "target"
 * field.
 */
static bool _match_cli_template(Cluster * cluster, Action * act, 
		regex_t * re, String expect)
{
    int n;
    int l;
    size_t nmatch = MAX_MATCH;
    regmatch_t pmatch[MAX_MATCH];
    int eflags = 0;
    char buf[MAX_BUF];
    char *str = str_get(expect);

    CHECK_MAGIC(act);

    re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
    n = Regexec(re, str, nmatch, pmatch, eflags);
    if (n != REG_NOERROR)
	return FALSE;
    if (pmatch[0].rm_so < 0)
	return FALSE;

    switch (act->com) {
    case PM_ERROR:
	assert(FALSE);
    case PM_LOG_IN:
    case PM_CHECK_LOGIN:
    case PM_LOG_OUT:
    case PM_UPDATE_PLUGS:
    case PM_UPDATE_NODES:
	return TRUE; /* don't need target data */
    case PM_POWER_ON:
    case PM_POWER_OFF:
    case PM_POWER_CYCLE:
    case PM_RESET:
    case PM_NAMES:
	assert((pmatch[1].rm_so >= 0) && (pmatch[1].rm_so < MAX_BUF));
	assert((pmatch[1].rm_eo >= 0) && (pmatch[1].rm_eo < MAX_BUF));
	l = pmatch[1].rm_eo - pmatch[1].rm_so;
	if (l == 0)
	    return FALSE;
	assert((l > 0) && (l < MAX_BUF));
	strncpy(buf, str + pmatch[1].rm_so, l);
	buf[l] = '\0';
	act->target = str_create(buf);
	return TRUE;
    default:
	assert(FALSE);
    }
    /*NOTREACHED*/
    return FALSE;
}


/*
 *   this function mostly will just increment the sequence number
 * and print another prompt.  If an update command is sent it
 * first sequences through the nodes send out the'r status as a
 * '0', '1', or '?'.
 */
void cli_reply(Cluster * cluster, Action * act)
{
    Client *client = act->client;
    int seq = act->seq;
    Node *node;
    ListIterator node_i;
    int n;
    size_t nm = MAX_MATCH;
    regmatch_t pm[MAX_MATCH];
    int eflags = 0;
    regex_t nre;
    reg_syntax_t cflags = REG_EXTENDED;

    CHECK_MAGIC(act);
    CHECK_MAGIC(client);
    assert(client->fd != NO_FD);

    client->write_status = CLI_WRITING;
    if ((act->client->loggedin == FALSE) && (act->com != PM_LOG_IN)) {
	buf_printf(client->to, "LOGIN FAILED\r\n%d Password> ", seq);
    } else {
	node_i = list_iterator_create(cluster->nodes);
	switch (act->com) {
	case PM_ERROR:
	    buf_printf(client->to, "ERROR\r\n%d PowerMan> ", seq);
	    break;
	case PM_LOG_IN:
	    act->client->loggedin = TRUE;
	case PM_CHECK_LOGIN:
	    buf_printf(client->to, "%d PowerMan> ", seq);
	    break;
	case PM_LOG_OUT:
	    break;
	case PM_UPDATE_PLUGS:
	    while ((node = list_next(node_i))) {
		if (node->p_state == ST_ON)
		    buf_printf(client->to, "1");
		else if (node->p_state == ST_OFF)
		    buf_printf(client->to, "0");
		else
		    buf_printf(client->to, "?");
	    }
	    buf_printf(client->to, "\r\n%d PowerMan> ", seq);
	    break;
	case PM_UPDATE_NODES:
	    while ((node = list_next(node_i))) {
		if (node->n_state == ST_ON)
		    buf_printf(client->to, "1");
		else if (node->n_state == ST_OFF)
		    buf_printf(client->to, "0");
		else
		    buf_printf(client->to, "?");
	    }
	    buf_printf(client->to, "\r\n%d PowerMan> ", seq);
	    break;
	case PM_POWER_ON:
	case PM_POWER_OFF:
	case PM_POWER_CYCLE:
	case PM_RESET:
	    buf_printf(client->to, "%d PowerMan> ", seq);
	    break;
	case PM_NAMES:
	    Regcomp(&nre, str_get(act->target), cflags);
	    while ((node = list_next(node_i))) {
		n = Regexec(&nre, str_get(node->name), nm, pm, eflags);
		if ((n != REG_NOMATCH)
		    && ((pm[0].rm_eo - pm[0].rm_so) ==
			str_length(node->name)))
		    buf_printf(client->to, "%s\r\n",
				str_get(node->name));
	    }
	    buf_printf(client->to, "%d PowerMan> ", seq);
	    break;
	default:
	    assert(FALSE);
	}
	list_iterator_destroy(node_i);
    }
}


Client *cli_create()
{
    Client *client;

    client = (Client *) Malloc(sizeof(Client));
    INIT_MAGIC(client);
    client->loggedin = FALSE;
    client->read_status = CLI_READING;
    client->write_status = CLI_IDLE;
    client->seq = 0;
    client->fd = NO_FD;
    client->ip = NULL;
    client->port = NO_PORT;
    client->host = NULL;
    client->to = NULL;
    client->from = NULL;
    return client;
}

/*
 *   This match utility is compatible with the list API's ListFindF
 * prototype for searching a list of Client * structs.  The match
 * criterion is an identity match on their addresses.  This comes 
 * into use in the act_find() when there is a chance an Action 
 * no longer has a client associated with it.
 */
int cli_match(Client * client, void *key)
{
    return (client == key);
}

void cli_destroy(Client * client)
{
    CHECK_MAGIC(client);

    if (client->fd != NO_FD) {
	Close(client->fd);
	syslog(LOG_DEBUG, "client on descriptor %d signs off",
	       ((Client *) client)->fd);
    }
    if (client->to != NULL)
	buf_destroy(client->to);
    if (client->from != NULL)
	buf_destroy(client->from);
    Free(client);
}


/*
 * vi:softtabstop=4
 */