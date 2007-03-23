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
 * $Id: uc.c,v 1.7 2007/03/23 10:46:35 adamdunkels Exp $
 */

/**
 * \file
 *         A brief description of what this file is.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "net/rime.h"
#include "net/rime/uc.h"
#include <string.h>

struct uc_hdr {
  rimeaddr_t receiver;
};

/*---------------------------------------------------------------------------*/
static void
recv_from_ibc(struct ibc_conn *ibc, rimeaddr_t *from)
{
  struct uc_conn *c = (struct uc_conn *)ibc;
  struct uc_hdr *hdr = rimebuf_dataptr();
  DEBUGF(2, "%d: uc: recv_from_ibc, receiver %d %p hdr %p\n",
	 rimeaddr_node_addr.u16, hdr->receiver.u16, c, hdr);
  if(rimeaddr_cmp(&hdr->receiver, &rimeaddr_node_addr)) {
    rimebuf_hdrreduce(sizeof(struct uc_hdr));
    c->u->recv(c, from);
  }
}
/*---------------------------------------------------------------------------*/
static const struct ibc_callbacks uc = {recv_from_ibc};
/*---------------------------------------------------------------------------*/
void
uc_open(struct uc_conn *c, u16_t channel,
	 const struct uc_callbacks *u)
{
  ibc_open(&c->c, channel, &uc);
  c->u = u;
}
/*---------------------------------------------------------------------------*/
void
uc_close(struct uc_conn *c)
{
  ibc_close(&c->c);
}
/*---------------------------------------------------------------------------*/
int
uc_send(struct uc_conn *c, rimeaddr_t *receiver)
{
  DEBUGF(2, "%d: uc_send to %d\n", rimeaddr_node_addr.u16, receiver->u16);
  if(rimebuf_hdralloc(sizeof(struct uc_hdr))) {
    struct uc_hdr *hdr = rimebuf_hdrptr();
    rimeaddr_copy(&hdr->receiver, receiver);
    return ibc_send(&c->c);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
