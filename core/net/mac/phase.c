/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * $Id: phase.c,v 1.7 2010/03/31 20:27:15 adamdunkels Exp $
 */

/**
 * \file
 *         Common functionality for phase optimization in duty cycling radio protocols
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "net/mac/phase.h"
#include "net/rime/packetbuf.h"
#include "sys/clock.h"
#include "lib/memb.h"
#include "net/rime/ctimer.h"
#include "net/rime/queuebuf.h"
#include "dev/watchdog.h"
#include "dev/leds.h"

struct phase_queueitem {
  struct ctimer timer;
  mac_callback_t mac_callback;
  void *mac_callback_ptr;
  struct queuebuf *q;
};

#define PHASE_DEFER_THRESHOLD 2
#define PHASE_QUEUESIZE       8

#define MAX_NOACKS            3

MEMB(phase_memb, struct phase_queueitem, PHASE_QUEUESIZE);

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTDEBUG(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#define PRINTDEBUG(...)
#endif
/*---------------------------------------------------------------------------*/
void
phase_update(const struct phase_list *list,
             const rimeaddr_t *neighbor, rtimer_clock_t time,
             int mac_status)
{
  struct phase *e;

  /* If we have an entry for this neighbor already, we renew it. */
  for(e = list_head(*list->list); e != NULL; e = e->next) {
    if(rimeaddr_cmp(neighbor, &e->neighbor)) {
      if(mac_status == MAC_TX_OK) {
        e->time = time;
      }

      /* If the neighbor didn't reply to us, it may have switched
         phase (rebooted). We try a number of transmissions to it
         before we drop it from the phase list. */
      if(mac_status == MAC_TX_NOACK) {
        PRINTF("phase noacks %d to %d.%d\n", e->noacks, neighbor->u8[0], neighbor->u8[1]);
        e->noacks++;
        if(e->noacks >= MAX_NOACKS) {
          list_remove(*list->list, e);
          memb_free(&phase_memb, e);
          return;
        }
      } else if(mac_status == MAC_TX_OK) {
        e->noacks = 0;
      }

      /* Make sure this entry is first on the list so subsequent
         searches are faster. */
      list_remove(*list->list, e);
      list_push(*list->list, e);
      break;
    }
  }
  /* No matching phase was found, so we allocate a new one. */
  if(mac_status == MAC_TX_OK && e == NULL) {
    e = memb_alloc(list->memb);
    if(e == NULL) {
      /* We could not allocate memory for this phase, so we drop
         the last item on the list and reuse it for our phase. */
      e = list_chop(*list->list);
    }
    rimeaddr_copy(&e->neighbor, neighbor);
    e->time = time;
    e->noacks = 0;
    list_push(*list->list, e);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  struct phase_queueitem *p = ptr;

  queuebuf_to_packetbuf(p->q);
  queuebuf_free(p->q);
  memb_free(&phase_memb, p);
  NETSTACK_RDC.send(p->mac_callback, p->mac_callback_ptr);
}
/*---------------------------------------------------------------------------*/
phase_status_t
phase_wait(struct phase_list *list,
           const rimeaddr_t *neighbor, rtimer_clock_t cycle_time,
           rtimer_clock_t wait_before,
           mac_callback_t mac_callback, void *mac_callback_ptr)
{
  struct phase *e;

  /* We go through the list of phases to find if we have recorded a
     phase for this particular neighbor. If so, we can compute the
     time for the next expected phase and setup a ctimer to switch on
     the radio just before the phase. */
  for(e = list_head(*list->list); e != NULL; e = e->next) {
    const rimeaddr_t *neighbor = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);

    if(rimeaddr_cmp(neighbor, &e->neighbor)) {
      rtimer_clock_t wait, now, expected, additional_wait;
      clock_time_t ctimewait;
      
      /* We expect phases to happen every CYCLE_TIME time
         units. The next expected phase is at time e->time +
         CYCLE_TIME. To compute a relative offset, we subtract
         with clock_time(). Because we are only interested in turning
         on the radio within the CYCLE_TIME period, we compute the
         waiting time with modulo CYCLE_TIME. */

      /*      printf("neighbor phase 0x%02x (cycle 0x%02x)\n", e->time & (cycle_time - 1),
              cycle_time);*/

      additional_wait = 2 * e->noacks * wait_before;

      /*      if(e->noacks > 0) {
        printf("additional wait %d\n", additional_wait);
        }*/

      now = RTIMER_NOW();
      wait = (rtimer_clock_t)((e->time - now) &
                              (cycle_time - 1));
      if(wait < wait_before + additional_wait) {
        wait += cycle_time;
      }
      
      ctimewait = (CLOCK_SECOND * (wait - wait_before - additional_wait)) / RTIMER_ARCH_SECOND;

      if(ctimewait > PHASE_DEFER_THRESHOLD) {
        struct phase_queueitem *p;

        p = memb_alloc(&phase_memb);
        if(p != NULL) {
          p->q = queuebuf_new_from_packetbuf();
          if(p->q != NULL) {
            p->mac_callback = mac_callback;
            p->mac_callback_ptr = mac_callback_ptr;
            ctimer_set(&p->timer, ctimewait, send_packet, p); 
            return PHASE_DEFERRED;
          } else {
            memb_free(&phase_memb, p);
          }
        }
      }
      
      expected = now + wait - wait_before - additional_wait;
      if(!RTIMER_CLOCK_LT(expected, now)) {
        /* Wait until the receiver is expected to be awake */
        while(RTIMER_CLOCK_LT(RTIMER_NOW(), expected)) {
        }
      }
      return PHASE_SEND_NOW;
    }
  }
  return PHASE_UNKNOWN;
}
/*---------------------------------------------------------------------------*/
void
phase_init(struct phase_list *list)
{
  list_init(*list->list);
  memb_init(list->memb);
  memb_init(&phase_memb);
}
/*---------------------------------------------------------------------------*/
