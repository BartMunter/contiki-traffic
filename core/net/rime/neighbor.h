/**
 * \addtogroup rime
 * @{
 */
/**
 * \defgroup rimeneighbor Rime neighbor management
 * @{
 *
 * The neighbor module manages the neighbor table.
 */
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
 * $Id: neighbor.h,v 1.5 2007/03/31 18:31:28 adamdunkels Exp $
 */

/**
 * \file
 *         Header file for the Contiki radio neighborhood management
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef __NEIGHBOR_H__
#define __NEIGHBOR_H__

#include "net/rime/rimeaddr.h"

struct neighbor {
  u16_t signal;
  u16_t time;
  rimeaddr_t addr;
  u8_t hopcount;
};

void neighbor_init(void);
/*void neighbor_periodic(int max_time);*/

void neighbor_add(rimeaddr_t *addr, u8_t hopcount, u16_t signal);
void neighbor_update(struct neighbor *n, u8_t hopcount, u16_t signal);

void neighbor_remove(rimeaddr_t *addr);

struct neighbor *neighbor_find(rimeaddr_t *addr);
struct neighbor *neighbor_best(void);
void neighbor_set_lifetime(int seconds);


#endif /* __NEIGHBOR_H__ */
/** @} */
/** @} */
