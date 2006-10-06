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
 * @(#)$Id: contiki-esb-default-init-net.c,v 1.3 2006/10/06 07:49:31 adamdunkels Exp $
 */

#include "contiki-esb.h"

static struct uip_fw_netif tr1001if =
  {UIP_FW_NETIF(0,0,0,0, 0,0,0,0, tr1001_drv_send)};

static struct uip_fw_netif slipif =
  {UIP_FW_NETIF(172,16,0,0, 255,255,255,0, slip_send)};

#define NODE_ID (node_id & 0xFF)

void
init_net(void)
{
  uip_ipaddr_t hostaddr;

  uip_init();
  uip_fw_init();
  
  rs232_set_input(slip_input_byte);

  tr1001_init();
  process_start(&tr1001_drv_process, NULL);
  process_start(&slip_process, NULL);
  
  if (NODE_ID > 0) {
    /* node id is set, construct an ip address based on the node id */
    uip_ipaddr(&hostaddr, 172, 16, 1, NODE_ID);
    uip_sethostaddr(&hostaddr);
  }
  uip_fw_register(&slipif);
  uip_fw_default(&tr1001if);
}
