/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * @(#)$Id: tapdev-service.c,v 1.2 2007/03/27 20:49:09 oliverschmidt Exp $
 */

#include "contiki-net.h"
#include "tapdev.h"
#include "net/uip-neighbor.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

static u8_t output(void);

SERVICE(tapdev_service, packet_service, { output });

PROCESS(tapdev_process, "TAP driver");

/*---------------------------------------------------------------------------*/
static u8_t
output(void)
{
  uip_arp_out();
  tapdev_send();
  
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
pollhandler(void)
{
  process_poll(&tapdev_process);
  uip_len = tapdev_poll();

  if(uip_len > 0) {
#if UIP_CONF_IPV6
    if(BUF->type == htons(UIP_ETHTYPE_IPV6)) {
      uip_neighbor_add(&IPBUF->srcipaddr, &BUF->src);
      tcpip_input();
    } else
#endif /* UIP_CONF_IPV6 */
    if(BUF->type == htons(UIP_ETHTYPE_IP)) {
      tcpip_input();
    } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
      uip_arp_arpin();
      /* If the above function invocation resulted in data that
	 should be sent out on the network, the global variable
	 uip_len is set to a value > 0. */
      if(uip_len > 0) {
	tapdev_do_send();
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tapdev_process, ev, data)
{
  PROCESS_BEGIN();

  tapdev_init();
  
  SERVICE_REGISTER(tapdev_service);

  process_poll(&tapdev_process);
  
  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_POLL) {
      pollhandler();
    }
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
