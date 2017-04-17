/*
 * (C) 2006-2007 by Pablo Neira Ayuso <pablo@netfilter.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* #include "conntrackd.h" */
#include "network.h"
/* #include "log.h" */

#include <stdlib.h>
#include <time.h>
#include <string.h>

#define NETHDR_ALIGNTO	4

static unsigned int seq_set, cur_seq;

int nethdr_align(int value)
{
	return (value + NETHDR_ALIGNTO - 1) & ~(NETHDR_ALIGNTO - 1);
}

int nethdr_size(int len)
{
	return NETHDR_SIZ + len;
}
	
static inline void __nethdr_set(struct nethdr *net, int len)
{
	if (!seq_set) {
		seq_set = 1;
		cur_seq = time(NULL);
	}
	net->version	= CONNTRACKD_PROTOCOL_VERSION;
	net->len	= len;
	net->seq	= cur_seq++;
}

void nethdr_set(struct nethdr *net, int type)
{
	__nethdr_set(net, NETHDR_SIZ);
	net->type = type;
}

void nethdr_set_ack(struct nethdr *net)
{
	__nethdr_set(net, NETHDR_ACK_SIZ);
}

void nethdr_set_ctl(struct nethdr *net)
{
	__nethdr_set(net, NETHDR_SIZ);
}


