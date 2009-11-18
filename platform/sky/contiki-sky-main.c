/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
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
 * @(#)$Id: contiki-sky-main.c,v 1.59 2009/11/18 13:51:55 nifi Exp $
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <io.h>

#include "contiki.h"

#include "dev/battery-sensor.h"
#include "dev/button-sensor.h"
#include "dev/cc2420.h"
#include "dev/ds2411.h"
#include "dev/leds.h"
#include "dev/light.h"
#include "dev/serial-line.h"
#include "dev/sht11.h"
#include "dev/slip.h"
#include "dev/uart1.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "net/mac/frame802154.h"
#include "net/mac/framer-802154.h"
#include "net/mac/framer-nullmac.h"
#include "net/mac/framer.h"

#if WITH_UIP6
#include "net/sicslowpan.h"
#include "net/uip-netif.h"
#include "net/mac/sicslowmac.h"
#if UIP_CONF_ROUTER
#include "net/routing/rimeroute.h"
#endif /* UIP_CONF_ROUTER*/
#endif /* WITH_UIP6 */

#include "net/rime.h"

#include "node-id.h"
#include "cfs-coffee-arch.h"
#include "cfs/cfs-coffee.h"
#include "sys/autostart.h"
#include "sys/profile.h"

SENSORS(&button_sensor);

#if DCOSYNCH_CONF_ENABLED
static struct timer mgt_timer;
#endif

#ifndef WITH_UIP
#define WITH_UIP 0
#endif

#if WITH_UIP
#include "net/uip.h"
#include "net/uip-fw.h"
#include "net/uip-fw-drv.h"
#include "net/uip-over-mesh.h"
static struct uip_fw_netif slipif =
  {UIP_FW_NETIF(192,168,1,2, 255,255,255,255, slip_send)};
static struct uip_fw_netif meshif =
  {UIP_FW_NETIF(172,16,0,0, 255,255,0,0, uip_over_mesh_send)};

#endif /* WITH_UIP */

#define UIP_OVER_MESH_CHANNEL 8
#if WITH_UIP
static uint8_t is_gateway;
#endif /* WITH_UIP */

#ifdef EXPERIMENT_SETUP
#include "experiment-setup.h"
#endif

#if WITH_NULLMAC
#define MAC_DRIVER nullmac_driver
#endif /* WITH_NULLMAC */

#ifndef MAC_DRIVER
#ifdef MAC_CONF_DRIVER
#define MAC_DRIVER MAC_CONF_DRIVER
#else
#define MAC_DRIVER xmac_driver
#endif /* MAC_CONF_DRIVER */
#endif /* MAC_DRIVER */

extern const struct mac_driver MAC_DRIVER;

/*---------------------------------------------------------------------------*/
#if 0
int
force_float_inclusion()
{
  extern int __fixsfsi;
  extern int __floatsisf;
  extern int __mulsf3;
  extern int __subsf3;

  return __fixsfsi + __floatsisf + __mulsf3 + __subsf3;
}
#endif
/*---------------------------------------------------------------------------*/
void uip_log(char *msg) { puts(msg); }
/*---------------------------------------------------------------------------*/
#ifndef RF_CHANNEL
#define RF_CHANNEL              26
#endif
/*---------------------------------------------------------------------------*/
void
force_inclusion(int d1, int d2)
{
  snprintf(NULL, 0, "%d", d1 % d2);
}
/*---------------------------------------------------------------------------*/
static void
set_rime_addr(void)
{
  rimeaddr_t addr;
  int i;

  memset(&addr, 0, sizeof(rimeaddr_t));
#if UIP_CONF_IPV6
  memcpy(addr.u8, ds2411_id, sizeof(addr.u8));
#else
  if(node_id == 0) {
    for(i = 0; i < sizeof(rimeaddr_t); ++i) {
      addr.u8[i] = ds2411_id[7 - i];
    }
  } else {
    addr.u8[0] = node_id & 0xff;
    addr.u8[1] = node_id >> 8;
  }
#endif
  rimeaddr_set_node_addr(&addr);
  printf("Rime started with address ");
  for(i = 0; i < sizeof(addr.u8) - 1; i++) {
    printf("%d.", addr.u8[i]);
  }
  printf("%d\n", addr.u8[i]);
}
/*---------------------------------------------------------------------------*/
static void
print_processes(struct process * const processes[])
{
  /*  const struct process * const * p = processes;*/
  printf("Starting");
  while(*processes != NULL) {
    printf(" '%s'", (*processes)->name);
    processes++;
  }
  putchar('\n');
}
/*--------------------------------------------------------------------------*/
#if WITH_UIP
static void
set_gateway(void)
{
  if(!is_gateway) {
    leds_on(LEDS_RED);
    printf("%d.%d: making myself the IP network gateway.\n\n",
	   rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
    printf("IPv4 address of the gateway: %d.%d.%d.%d\n\n",
	   uip_ipaddr_to_quad(&uip_hostaddr));
    uip_over_mesh_set_gateway(&rimeaddr_node_addr);
    uip_over_mesh_make_announced_gateway();
    is_gateway = 1;

    rime_mac->off(1);
  }
}
#endif /* WITH_UIP */
/*---------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  /*
   * Initalize hardware.
   */
  msp430_cpu_init();
  clock_init();
  leds_init();
  leds_on(LEDS_RED);

  uart1_init(BAUD2UBR(115200)); /* Must come before first printf */
#if WITH_UIP
  slip_arch_init(BAUD2UBR(115200));
#endif /* WITH_UIP */

  leds_on(LEDS_GREEN);
  ds2411_init();

  /* XXX hack: Fix it so that the 802.15.4 MAC address is compatible
     with an Ethernet MAC address - byte 0 (byte 2 in the DS ID)
     cannot be odd. */
  ds2411_id[2] &= 0xfe;
  
  leds_on(LEDS_BLUE);
  xmem_init();

  leds_off(LEDS_RED);
  rtimer_init();
  /*
   * Hardware initialization done!
   */

  
  /* Restore node id if such has been stored in external mem */
  node_id_restore();

  random_init(ds2411_id[0] + node_id);
  
  leds_off(LEDS_BLUE);
  /*
   * Initialize Contiki and our processes.
   */
  process_init();
  process_start(&etimer_process, NULL);
  process_start(&sensors_process, NULL);

  /*
   * Initialize light and humidity/temp sensors.
   */
  sensors_light_init();
  battery_sensor.activate();
  sht11_init();

  ctimer_init();

  cc2420_init();
  cc2420_set_pan_addr(IEEE802154_PANID, 0 /*XXX*/, ds2411_id);
  cc2420_set_channel(RF_CHANNEL);

  printf(CONTIKI_VERSION_STRING " started. ");
  if(node_id > 0) {
    printf("Node id is set to %u.\n", node_id);
  } else {
    printf("Node id is not set.\n");
  }
  set_rime_addr();
  printf("MAC %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
	 ds2411_id[0], ds2411_id[1], ds2411_id[2], ds2411_id[3],
	 ds2411_id[4], ds2411_id[5], ds2411_id[6], ds2411_id[7]);

  framer_set(&framer_802154);
  
#if WITH_UIP6
  memcpy(&uip_lladdr.addr, ds2411_id, sizeof(uip_lladdr.addr));
  /* Setup nullmac-like MAC for 802.15.4 */
/*   sicslowpan_init(sicslowmac_init(&cc2420_driver)); */
/*   printf(" %s channel %u\n", sicslowmac_driver.name, RF_CHANNEL); */

  /* Setup X-MAC for 802.15.4 */
  queuebuf_init();
  sicslowpan_init(MAC_DRIVER.init(&cc2420_driver));
  printf(" %s channel %u\n", MAC_DRIVER.name, RF_CHANNEL);

  process_start(&tcpip_process, NULL);

  printf("Tentative link-local IPv6 address ");
  {
    int i;
    for(i = 0; i < 7; ++i) {
      printf("%02x%02x:",
	     uip_netif_physical_if.addresses[0].ipaddr.u8[i * 2],
	     uip_netif_physical_if.addresses[0].ipaddr.u8[i * 2 + 1]);
    }
    printf("%02x%02x\n",
	   uip_netif_physical_if.addresses[0].ipaddr.u8[14],
	   uip_netif_physical_if.addresses[0].ipaddr.u8[15]);
  }
  
#if UIP_CONF_ROUTER
  uip_router_register(&rimeroute);
#endif /* UIP_CONF_ROUTER */
#else /* WITH_UIP6 */
  rime_init(MAC_DRIVER.init(&cc2420_driver));
  printf(" %s channel %u\n", rime_mac->name, RF_CHANNEL);
#endif /* WITH_UIP6 */

#if !WITH_UIP && !WITH_UIP6
  uart1_set_input(serial_line_input_byte);
  serial_line_init();
#endif

#if PROFILE_CONF_ON
  profile_init();
#endif /* PROFILE_CONF_ON */

  leds_off(LEDS_GREEN);

#if TIMESYNCH_CONF_ENABLED
  timesynch_init();
  timesynch_set_authority_level(rimeaddr_node_addr.u8[0]);
#endif /* TIMESYNCH_CONF_ENABLED */

#if WITH_UIP
  process_start(&tcpip_process, NULL);
  process_start(&uip_fw_process, NULL);	/* Start IP output */
  process_start(&slip_process, NULL);

  slip_set_input_callback(set_gateway);

  {
    uip_ipaddr_t hostaddr, netmask;

    uip_init();

    uip_ipaddr(&hostaddr, 172,16,
	       rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1]);
    uip_ipaddr(&netmask, 255,255,0,0);
    uip_ipaddr_copy(&meshif.ipaddr, &hostaddr);

    uip_sethostaddr(&hostaddr);
    uip_setnetmask(&netmask);
    uip_over_mesh_set_net(&hostaddr, &netmask);
    /*    uip_fw_register(&slipif);*/
    uip_over_mesh_set_gateway_netif(&slipif);
    uip_fw_default(&meshif);
    uip_over_mesh_init(UIP_OVER_MESH_CHANNEL);
    printf("uIP started with IP address %d.%d.%d.%d\n",
	   uip_ipaddr_to_quad(&hostaddr));
  }
#endif /* WITH_UIP */

  button_sensor.activate();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  print_processes(autostart_processes);
  autostart_start(autostart_processes);

  /*
   * This is the scheduler loop.
   */
#if DCOSYNCH_CONF_ENABLED
  timer_set(&mgt_timer, DCOSYNCH_PERIOD * CLOCK_SECOND);
#endif
  watchdog_start();
  /*  watchdog_stop();*/
  while(1) {
    int r;
#if PROFILE_CONF_ON
    profile_episode_start();
#endif /* PROFILE_CONF_ON */
    do {
      /* Reset watchdog. */
      watchdog_periodic();
      r = process_run();
    } while(r > 0);
#if PROFILE_CONF_ON
    profile_episode_end();
#endif /* PROFILE_CONF_ON */

    /*
     * Idle processing.
     */
    int s = splhigh();		/* Disable interrupts. */
    /* uart1_active is for avoiding LPM3 when still sending or receiving */
    if(process_nevents() != 0 || uart1_active()) {
      splx(s);			/* Re-enable interrupts. */
    } else {
      static unsigned long irq_energest = 0;

#if DCOSYNCH_CONF_ENABLED
      /* before going down to sleep possibly do some management */
      if (timer_expired(&mgt_timer)) {
	timer_reset(&mgt_timer);
	msp430_sync_dco();
      }
#endif

      /* Re-enable interrupts and go to sleep atomically. */
      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      ENERGEST_ON(ENERGEST_TYPE_LPM);
      /* We only want to measure the processing done in IRQs when we
	 are asleep, so we discard the processing time done when we
	 were awake. */
      energest_type_set(ENERGEST_TYPE_IRQ, irq_energest);
      watchdog_stop();
      _BIS_SR(GIE | SCG0 | SCG1 | CPUOFF); /* LPM3 sleep. This
					      statement will block
					      until the CPU is
					      woken up by an
					      interrupt that sets
					      the wake up flag. */

      /* We get the current processing time for interrupts that was
	 done during the LPM and store it for next time around.  */
      dint();
      irq_energest = energest_type_time(ENERGEST_TYPE_IRQ);
      eint();
      watchdog_start();
      ENERGEST_OFF(ENERGEST_TYPE_LPM);
      ENERGEST_ON(ENERGEST_TYPE_CPU);
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
#if LOG_CONF_ENABLED
void
log_message(char *m1, char *m2)
{
  printf("%s%s\n", m1, m2);
}
#endif /* LOG_CONF_ENABLED */
