/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
 * $Id: sky-shell.c,v 1.24 2010/10/03 20:16:57 adamdunkels Exp $
 */

/**
 * \file
 *         Tmote Sky-specific Contiki shell
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "powertrace.h"
#include "contiki.h"
#include "shell.h"
#include "serial-shell.h"

#include "net/rime/collect-neighbor.h"
#include "dev/watchdog.h"

#include "net/rime.h"
#include "dev/cc2420.h"
#include "dev/leds.h"
#include "dev/light-sensor.h"
#include "dev/battery-sensor.h"
#include "dev/sht11-sensor.h"

#include "lib/checkpoint.h"

#include "net/rime/timesynch.h"

#include "powertrace.h"

#include <stdio.h>
#include <string.h>

#include <io.h>
#include <signal.h>

#define DEBUG_SNIFFERS 0

/*---------------------------------------------------------------------------*/
PROCESS(sky_shell_process, "Sky Contiki shell");
AUTOSTART_PROCESSES(&sky_shell_process);
/*---------------------------------------------------------------------------*/
#if DEBUG_SNIFFERS
static void
input_sniffer(void)
{
  int i;
  uint8_t *dataptr;

  printf("x %d ", packetbuf_totlen());
  dataptr = packetbuf_dataptr();
  printf("%02x ", dataptr[0]);
  /*  if(dataptr[0] == 18) {*/
    for(i = 1; i < packetbuf_totlen(); ++i) {
      printf("%02x ", dataptr[i]);
    }
    /*  }*/
  printf("\n");
}
/*---------------------------------------------------------------------------*/
static void
output_sniffer(void)
{
  uint8_t *dataptr;
  printf("- %d ", packetbuf_totlen());
  dataptr = packetbuf_dataptr();
  printf("%02x\n", dataptr[0]);
}
/*---------------------------------------------------------------------------*/
RIME_SNIFFER(s, input_sniffer, output_sniffer);
#endif /* DEBUG_SNIFFERS */
/*---------------------------------------------------------------------------*/
PROCESS(shell_sky_alldata_process, "sky-alldata");
SHELL_COMMAND(sky_alldata_command,
	      "sky-alldata",
	      "sky-alldata: sensor data, power consumption, network stats",
	      &shell_sky_alldata_process);
/*---------------------------------------------------------------------------*/
#define MAX(a, b) ((a) > (b)? (a): (b))
#define MIN(a, b) ((a) < (b)? (a): (b))
struct spectrum {
  int channel[16];
};
#define NUM_SAMPLES 4
static struct spectrum rssi_samples[NUM_SAMPLES];
static int
do_rssi(void)
{
  static int sample;
  int channel;
  
  rime_mac->off(0);

  cc2420_on();
  for(channel = 11; channel <= 26; ++channel) {
    cc2420_set_channel(channel);
    rssi_samples[sample].channel[channel - 11] = cc2420_rssi() + 53;
  }
  
  rime_mac->on();
  
  sample = (sample + 1) % NUM_SAMPLES;

  {
    int channel, tot;
    tot = 0;
    for(channel = 0; channel < 16; ++channel) {
      int max = -256;
      int i;
      for(i = 0; i < NUM_SAMPLES; ++i) {
	max = MAX(max, rssi_samples[i].channel[channel]);
      }
      tot += max / 20;
    }
    return tot;
  }
}
/*---------------------------------------------------------------------------*/
struct sky_alldata_msg {
  uint16_t len;
  uint16_t clock;
  uint16_t timesynch_time;
  uint16_t light1;
  uint16_t light2;
  uint16_t temp;
  uint16_t humidity;
  uint16_t rssi;
  uint16_t cpu;
  uint16_t lpm;
  uint16_t transmit;
  uint16_t listen;
  rimeaddr_t parent;
  uint16_t parent_etx;
  uint16_t parent_rtmetric;
  uint16_t battery_voltage;
  uint16_t battery_indicator;
  uint16_t num_neighbors;
};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_sky_alldata_process, ev, data)
{
  static unsigned long last_cpu, last_lpm, last_transmit, last_listen;
  unsigned long cpu, lpm, transmit, listen;
  struct sky_alldata_msg msg;
  struct collect_neighbor *n;
  PROCESS_BEGIN();


  SENSORS_ACTIVATE(light_sensor);
  SENSORS_ACTIVATE(battery_sensor);
  SENSORS_ACTIVATE(sht11_sensor);
  
  msg.len = sizeof(struct sky_alldata_msg) / sizeof(uint16_t);
  msg.clock = clock_time();
#if TIMESYNCH_CONF_ENABLED
  msg.timesynch_time = timesynch_time();
#else /* TIMESYNCH_CONF_ENABLED */
  msg.timesynch_time = 0;
#endif /* TIMESYNCH_CONF_ENABLED */
  msg.light1 = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
  msg.light2 = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
  msg.temp = sht11_sensor.value(SHT11_SENSOR_TEMP);
  msg.humidity = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
  
  energest_flush();
  
  cpu = energest_type_time(ENERGEST_TYPE_CPU) - last_cpu;
  lpm = energest_type_time(ENERGEST_TYPE_LPM) - last_lpm;
  transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT) - last_transmit;
  listen = energest_type_time(ENERGEST_TYPE_LISTEN) - last_listen;

  /* Make sure that the values are within 16 bits. */
  while(cpu >= 65536ul || lpm >= 65536ul ||
	transmit >= 65536ul || listen >= 65536ul) {
    cpu /= 2;
    lpm /= 2;
    transmit /= 2;
    listen /= 2;
  }
  
  msg.cpu = cpu;
  msg.lpm = lpm;
  msg.transmit = transmit;
  msg.listen = listen;

  last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  last_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  last_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

  rimeaddr_copy(&msg.parent, &shell_collect_conn.parent);
  n = collect_neighbor_list_find(&shell_collect_conn.neighbor_list,
                                 &shell_collect_conn.parent);
  if(n != NULL) {
    msg.parent_etx = collect_neighbor_link_estimate(n);
    msg.parent_rtmetric = n->rtmetric;
  } else {
    msg.parent_etx = 0;
    msg.parent_rtmetric = 0;
  }
  msg.battery_voltage = battery_sensor.value(0);
  msg.battery_indicator = sht11_sensor.value(SHT11_SENSOR_BATTERY_INDICATOR);
  msg.num_neighbors = collect_neighbor_list_num(&shell_collect_conn.neighbor_list);
  shell_output(&sky_alldata_command, &msg, sizeof(msg), "", 0);
  
  SENSORS_DEACTIVATE(light_sensor);
  SENSORS_DEACTIVATE(battery_sensor);
  SENSORS_DEACTIVATE(sht11_sensor);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
periodic_debug(void *ptr)
{
  ctimer_set(ptr, 20 * CLOCK_SECOND, periodic_debug, ptr);
  collect_print_stats();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sky_shell_process, ev, data)
{
  static struct ctimer c;
  PROCESS_BEGIN();

  /*  ctimer_set(&c, 20 * CLOCK_SECOND, periodic_debug, &c);*/
  
  /*  powertrace_start(30 * CLOCK_SECOND);
      powertrace_sniff(POWERTRACE_ON);*/
  
  serial_shell_init();
  shell_blink_init();
  /*  shell_file_init();
      shell_coffee_init();*/
  /*  shell_download_init();
      shell_rime_sendcmd_init();*/
  /*  shell_ps_init();*/
  shell_reboot_init();
  shell_rime_init();
  shell_rime_netcmd_init();
  /*  shell_rime_ping_init();
  shell_rime_debug_init();
  shell_rime_debug_runicast_init();*/
  /*  shell_rime_sniff_init();*/
  shell_sky_init();
  shell_power_init();
  /*  shell_base64_init();*/
  shell_text_init();
  shell_time_init();
  /*  shell_checkpoint_init();*/
  /*  shell_sendtest_init();*/

  shell_register_command(&sky_alldata_command);

#if DEBUG_SNIFFERS
  rime_sniffer_add(&s);
#endif /* DEBUG_SNIFFERS */
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
