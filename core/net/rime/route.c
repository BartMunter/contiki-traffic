/**
 * \addtogroup rimeroute
 * @{
 */

/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * $Id: route.c,v 1.6 2007/03/31 18:31:28 adamdunkels Exp $
 */

/**
 * \file
 *         Rime route table
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <stdio.h>

#include "net/rime/route.h"

#define NUM_RT_ENTRIES 8

/*
 * LRU (with respect to insertion time) list of route entries.
 */
LIST(route_table);
MEMB(route_mem, struct route_entry, NUM_RT_ENTRIES);

static struct ctimer t;

static int max_time = 10;

/*---------------------------------------------------------------------------*/
static void
periodic(void *ptr)
{
  struct route_entry *e;

  for(e = list_head(route_table); e != NULL; e = e->next) {
    e->time++;
    if(e->time >= max_time) {
      printf("Route to %d.%d bropped\n",
	     e->dest.u8[0], e->dest.u8[1]);
      list_remove(route_table, e);
      memb_free(&route_mem, e);
    }
  }

  ctimer_set(&t, CLOCK_SECOND, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
void
route_init(void)
{
  list_init(route_table);
  memb_init(&route_mem);

  ctimer_set(&t, CLOCK_SECOND, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
int
route_add(rimeaddr_t *dest, rimeaddr_t *nexthop,
	  u8_t hop_count, u8_t seqno)
{
  struct route_entry *e;

  /* Avoid inserting duplicate entries. */
  e = route_lookup(dest);
  if(e != NULL) {
    list_remove(route_table, e);    
  } else {
    /* Allocate a new entry or reuse the oldest. */
    e = memb_alloc(&route_mem);
    if(e == NULL) {
      e = list_chop(route_table); /* Remove oldest entry. */
    }
  }

  rimeaddr_copy(&e->dest, dest);
  rimeaddr_copy(&e->nexthop, nexthop);
  e->hop_count = hop_count;
  e->seqno = seqno;
  e->time = 0;

  /* New entry goes first. */
  list_push(route_table, e);

  return 0;
}
/*---------------------------------------------------------------------------*/
struct route_entry *
route_lookup(rimeaddr_t *dest)
{
  struct route_entry *e;

  for(e = list_head(route_table); e != NULL; e = e->next) {
    /*    printf("route_lookup: comparing %d.%d.%d.%d with %d.%d.%d.%d\n",
	   uip_ipaddr1(dest),
	   uip_ipaddr2(dest),
	   uip_ipaddr3(dest),
	   uip_ipaddr4(dest),
	   uip_ipaddr1(&e->dest),
	   uip_ipaddr2(&e->dest),
	   uip_ipaddr3(&e->dest),
	   uip_ipaddr4(&e->dest));*/
	   
    if(rimeaddr_cmp(dest, &e->dest)) {
      return e;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
route_remove(struct route_entry *e)
{
  list_remove(route_table, e);
  memb_free(&route_mem, e);
}
/*---------------------------------------------------------------------------*/
void
route_flush_all(void)
{
  struct route_entry *e;

  while(1) {
    e = list_pop(route_table);
    if(e != NULL) {
      memb_free(&route_mem, e);
    } else {
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
route_set_lifetime(int seconds)
{
  max_time = seconds;
}
/*---------------------------------------------------------------------------*/
/** @} */
