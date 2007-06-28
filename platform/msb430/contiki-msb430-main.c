/*
 * Copyright (c) 2007, Swedish Institute of Computer Science
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
 */

/**
 * \file
 *	Main system file for the MSB-430 port.
 * \author
 * 	Michael Baar <baar@inf.fu-berlin.de>, Nicolas Tsiftes <nvt@sics.se>
 */

#include <io.h>
#include <signal.h>

#include <stdio.h>
#include <string.h>

#include "contiki-msb430.h"

#include "sys/procinit.h"
#include "sys/autostart.h"

#include "dev/adc.h"

#include "net/mac/nullmac.h"
#include "net/mac/xmac.h"

SENSORS(NULL);

static void
set_rime_addr(void)
{
  rimeaddr_t addr;
  addr.u16[0] = node_id;
  rimeaddr_set_node_addr(&addr);
}

static void
msb_ports_init(void)
{
  P1SEL = 0x00;
  P1OUT = 0x00;
  P1DIR = 0x00;

  P2SEL = 0x00;
  P2OUT = 0x18;
  P2DIR = 0x1A;

  P3SEL = 0x00;
  P3OUT = 0x09;
  P3DIR = 0x21;

  P4SEL = 0x00;
  P4OUT = 0x00;
  P4DIR = 0x00;

  P5SEL = 0x0E;
  P5OUT = 0xF9;
  P5DIR = 0xFD;

  P6SEL = 0x07;
  P6OUT = 0x00;
  P6DIR = 0xC8;
}

int
main(void)
{
  msp430_cpu_init();	

  /* Platform-specific initialization. */
  msb_ports_init();
  adc_init();

  clock_init();
  leds_init();
  leds_on(LEDS_ALL);

  // low level
  irq_init();
  process_init();

  // serial interface
  rs232_init();

#ifdef WITH_SDC
  spi_init();
#endif

  uart_lock(UART_MODE_RS232);
  uart_unlock(UART_MODE_RS232);
  //slip_arch_init(BAUD2UBR(115200));

  cc1020_init(cc1020_config_19200);

  // network configuration
  node_id_restore();

  nullmac_init(&cc1020_driver);
  rime_init(&nullmac_driver);
  set_rime_addr();

  /* System services */
  process_start(&etimer_process, NULL);
  //process_start(&sensors_process, NULL);

  leds_off(LEDS_ALL);
  
  printf("Autostarting processes\n");
  autostart_start((struct process **) autostart_processes);

  for (;;) {
    while (process_run());
    if (process_nevents() == 0)
      LPM1;
  }

  return 0;
}
