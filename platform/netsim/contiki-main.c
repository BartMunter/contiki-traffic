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
 * $Id: contiki-main.c,v 1.17 2007/10/25 08:26:49 zhitao Exp $
 */

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"

#include "net/rime.h"

#ifdef __CYGWIN__
#include "net/wpcap-drv.h"
#else
#include "net/tapdev-drv.h"
#endif
#include "net/ethernode-uip.h"
#include "net/ethernode-rime.h"
#include "net/ethernode.h"
#include "net/uip-over-mesh.h"

#include "ether.h"

/*#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>*/

#include "dev/button-sensor.h"
#include "dev/pir-sensor.h"
#include "dev/vib-sensor.h"
#include "dev/radio-sensor.h"
#include "dev/leds.h"

#ifdef __CYGWIN__
static struct uip_fw_netif extif =
  {UIP_FW_NETIF(0,0,0,0, 0,0,0,0, wpcap_output)};
#else
static struct uip_fw_netif extif =
  {UIP_FW_NETIF(0,0,0,0, 0,0,0,0, tapdev_output)};
#endif
static struct uip_fw_netif meshif =
  {UIP_FW_NETIF(172,16,0,0, 255,255,0,0, uip_over_mesh_send)};
/*static struct uip_fw_netif ethernodeif =
  {UIP_FW_NETIF(172,16,0,0, 255,255,0,0, ethernode_drv_send)};*/

static const struct uip_eth_addr ethaddr = {{0x00,0x06,0x98,0x01,0x02,0x12}};

SENSORS(&button_sensor, &pir_sensor, &vib_sensor, &radio_sensor);

PROCINIT(&sensors_process, &etimer_process, &tcpip_process,
	 /*	 &ethernode_uip_process,*/
	 &uip_fw_process);

/*---------------------------------------------------------------------------*/
void
contiki_main(int flag)
{
  random_init(getpid());
  srand(getpid());

  leds_init();
  
  process_init();

  procinit_init();

  uip_init();
  
  rime_init(nullmac_init(&ethernode_driver));

  uip_over_mesh_init(0);
  
  if(flag == 1) {
#ifdef __CYGWIN__
    process_start(&wpcap_process, NULL);
#else
    process_start(&tapdev_process, NULL);
#endif
    uip_fw_register(&meshif);
    uip_fw_default(&extif);
    printf("uip_hostaddr %02x%02x\n", uip_hostaddr.u16[0], uip_hostaddr.u16[1]);
  } else {
    uip_fw_default(&meshif);
  }
  
  leds_green(LEDS_ON);

  rtimer_init();
  
  autostart_start(autostart_processes);
  
  while(1) {
    int n;
    n = process_run();
    /*    if(n > 0) {
      printf("%d processes in queue\n");
      }*/
    usleep(1);
    etimer_request_poll();
  }

}
/*---------------------------------------------------------------------------*/
process_event_t codeprop_event_quit;
