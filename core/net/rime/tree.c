/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * $Id: tree.c,v 1.6 2007/03/22 18:54:22 adamdunkels Exp $
 */

/**
 * \file
 *         A brief description of what this file is.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"

#include "net/rime.h"
#include "net/rime/neighbor.h"
#include "net/rime/tree.h"

#include "dev/radio-sensor.h"

#if NETSIM
#include "ether.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stddef.h>

struct adv_msg {
  u8_t hopcount;
  u8_t pad;
};

struct hdr {
  rimeaddr_t originator;
  u8_t originator_seqno;
  u8_t hopcount;
  u8_t hoplim;
  u8_t retransmissions;
  u8_t datalen;
  u8_t data[1];
};

#define SINK 0
#define HOPCOUNT_MAX TREE_MAX_DEPTH

#define MAX_HOPLIM 10

#define MAX_INTERVAL CLOCK_SECOND * 4

/*---------------------------------------------------------------------------*/
static void
update_hopcount(struct tree_conn *tc)
{
  struct neighbor *n;
  
  if(tc->hops_from_sink != SINK) {
    n = neighbor_best();
    if(n == NULL) {
      /*      if(hopcount != HOPCOUNT_MAX) {
	printf("%d: didn't find a best neighbor, setting hopcount to max\n", node_id);
	}*/
      tc->hops_from_sink = HOPCOUNT_MAX;
    } else {
      if(n->hopcount + 1 != tc->hops_from_sink) {
	tc->hops_from_sink = n->hopcount + 1;
      }
    }
  }

  /*  DEBUG_PRINTF("%d: new hopcount %d\n", node_id, hopcount);*/
#if NETSIM
  {
    char buf[8];
    if(tc->hops_from_sink == HOPCOUNT_MAX) {
      strcpy(buf, " ");
    } else {
      snprintf(buf, sizeof(buf), "%d", tc->hops_from_sink);
    }
    ether_set_text(buf);
  }
#endif
}
/*---------------------------------------------------------------------------*/
static void
send_adv(struct tree_conn *tc)
{
  struct adv_msg *hdr;

  rimebuf_clear();
  rimebuf_set_datalen(sizeof(struct adv_msg));
  hdr = rimebuf_dataptr();
  hdr->hopcount = tc->hops_from_sink;
  uibc_send(&tc->uibc_conn, MAX_INTERVAL);
}
/*---------------------------------------------------------------------------*/
static void
adv_packet_received(struct uibc_conn *c, rimeaddr_t *from)
{
  struct tree_conn *tc = (struct tree_conn *)
    ((char *)c - offsetof(struct tree_conn, uibc_conn));
  struct adv_msg *msg = rimebuf_dataptr();
  struct neighbor *n;

  /*  printf("%d: neighbor_packet_received from %d with hopcount %d\n",
	 node_id, from, hdr->hopcount
	 );*/
  
  n = neighbor_find(from);

  if(n == NULL) {
    neighbor_add(from, msg->hopcount, radio_sensor.value(1));
  } else {
    neighbor_update(n, msg->hopcount, radio_sensor.value(1));
  }

  update_hopcount(tc);

}
/*---------------------------------------------------------------------------*/
static void
adv_packet_sent(struct uibc_conn *c)
{
  struct tree_conn *tc = (struct tree_conn *)
    ((char *)c - offsetof(struct tree_conn, uibc_conn));
  send_adv(tc);
}
/*---------------------------------------------------------------------------*/
static void
adv_packet_dropped(struct uibc_conn *c)
{
  struct tree_conn *tc = (struct tree_conn *)
    ((char *)c - offsetof(struct tree_conn, uibc_conn));
  send_adv(tc);
}
/*---------------------------------------------------------------------------*/
static int
node_packet_received(struct ruc_conn *c, rimeaddr_t *from, u8_t seqno)
{
  struct tree_conn *tc = (struct tree_conn *)
    ((char *)c - offsetof(struct tree_conn, ruc_conn));
  struct hdr *hdr = rimebuf_dataptr();
  struct neighbor *n;

  if(tc->hops_from_sink == SINK) {

    if(tc->cb->recv != NULL) {
      tc->cb->recv(&hdr->originator, hdr->originator_seqno,
		   hdr->hopcount, hdr->retransmissions);
    }
    return 1;
  } else if(hdr->hoplim > 1 && tc->hops_from_sink != HOPCOUNT_MAX) {
    hdr->hoplim--;
    /*    printf("%d: packet received from %d, forwarding %d, best neighbor %p\n",
	  rimeaddr_node_addr.u16[0], from->u16[0], tc->forwarding, neighbor_best());*/
    if(!tc->forwarding) {
      tc->forwarding = 1;
      n = neighbor_best();
      if(n != NULL) {
	ruc_send(c, &n->addr);
      }
      return 1;
    } else {

      /*      printf("%d: still forwarding another packet, not sending ACK\n",
	      rimeaddr_node_addr.u16[0]);*/
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
node_packet_sent(struct ruc_conn *c)
{
  struct tree_conn *tc = (struct tree_conn *)
    ((char *)c - offsetof(struct tree_conn, ruc_conn));

  tc->forwarding = 0;
}
/*---------------------------------------------------------------------------*/
static const struct uibc_callbacks uibc_callbacks =
  {adv_packet_received, adv_packet_sent, adv_packet_dropped};
static const struct ruc_callbacks ruc_callbacks = {node_packet_received,
					     node_packet_sent};
/*---------------------------------------------------------------------------*/
void
tree_open(struct tree_conn *tc, u16_t channels,
	  const struct tree_callbacks *cb)
{
  uibc_open(&tc->uibc_conn, channels, &uibc_callbacks);
  ruc_open(&tc->ruc_conn, channels + 1, &ruc_callbacks);
  tc->hops_from_sink = HOPCOUNT_MAX;
  /*  rimebuf_clear();
  rimebuf_reference(&tc.hello, sizeof(tc.hello));
  sibc_send_stubborn(&sibc_conn, CLOCK_SECOND * 8);*/
  tc->cb = cb;
  send_adv(tc);
}
/*---------------------------------------------------------------------------*/
void
tree_close(struct tree_conn *tc)
{
  uibc_close(&tc->uibc_conn);
  ruc_close(&tc->ruc_conn);
}
/*---------------------------------------------------------------------------*/
void
tree_set_sink(struct tree_conn *tc, int should_be_sink)
{
  if(should_be_sink) {
    tc->hops_from_sink = SINK;
  } else {
    tc->hops_from_sink = HOPCOUNT_MAX;
  }
  update_hopcount(tc);
}
/*---------------------------------------------------------------------------*/
void
tree_send(struct tree_conn *tc)
{
  struct neighbor *n;
  struct hdr *hdr;

  if(rimebuf_hdrextend(sizeof(struct hdr))) {
    hdr = rimebuf_hdrptr();
    hdr->originator_seqno = tc->seqno++;
    rimeaddr_copy(&hdr->originator, &rimeaddr_node_addr);
    hdr->hopcount = tc->hops_from_sink;
    hdr->hoplim = MAX_HOPLIM;
    hdr->retransmissions = 0;
    hdr->datalen = 0;
    n = neighbor_best();
    if(n != NULL) {
      /*      printf("Sending to best neighbor\n");*/
      ruc_send(&tc->ruc_conn, &n->addr);
    } else {
      /*      printf("Didn't find any neighbor\n");*/
    }
  }
}
/*---------------------------------------------------------------------------*/
int
tree_depth(struct tree_conn *tc)
{
  return tc->hops_from_sink;
}
/*---------------------------------------------------------------------------*/
