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
 * $Id: trickle.h,v 1.2 2007/03/21 23:23:02 adamdunkels Exp $
 */

/**
 * \file
 *         Header file for Trickle (reliable single source flooding) for Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef __TRICKLE_H__
#define __TRICKLE_H__

#include "net/rime.h"

struct trickle_conn;

struct trickle_callbacks {
  void (* recv)(struct trickle_conn *c);
};

struct trickle_conn {
  struct abc_conn c;
  const struct trickle_callbacks *cb;
  struct queuebuf *q;
  struct ctimer intervaltimer;
  struct ctimer timer;
  struct pt pt;
  u8_t interval;
  u8_t seqno;
  u8_t count;
  u8_t interval_scaling;
};

void trickle_open(struct trickle_conn *c, u16_t channel,
		  const struct trickle_callbacks *cb);
void trickle_close(struct trickle_conn *c);

#define TRICKLE_SECOND 8
void trickle_send(struct trickle_conn *c, u8_t interval);

#endif /* __TRICKLE_H__ */
