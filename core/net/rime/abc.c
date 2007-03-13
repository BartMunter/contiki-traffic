/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: abc.c,v 1.1 2007/03/13 13:01:48 adamdunkels Exp $
 */

/**
 * \addtogroup rime
 * @{
 */

/**
 * \file
 *         Anonymous best-effort local area Broad Cast (abc)
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime.h"

struct abc_hdr {
  u16_t channel;
};

LIST(channels);

/*---------------------------------------------------------------------------*/
void
abc_setup(struct abc_conn *c, u16_t channel,
	  const struct abc_ulayer *ulayer)
{
  struct channel *chan;

  c->channel = channel;
  c->u = ulayer;

  list_add(channels, c);
}
/*---------------------------------------------------------------------------*/
int
abc_send(struct abc_conn *c)
{
  if(rimebuf_hdrextend(sizeof(struct abc_hdr))) {
    struct abc_hdr *hdr = rimebuf_hdrptr();

    hdr->channel = c->channel;
    rimebuf_copy_reference();
    abc_arch_send(rimebuf_hdrptr(), rimebuf_totlen());
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
abc_input_packet(void)
{
  struct abc_hdr *hdr;
  struct abc_conn *c;

  hdr = rimebuf_dataptr();
  
  if(rimebuf_hdrreduce(sizeof(struct abc_hdr))) {

    for(c = list_head(channels); c != NULL; c = c->next) {
      if(c->channel == hdr->channel) {
	c->u->recv(c);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/

/** @} */
