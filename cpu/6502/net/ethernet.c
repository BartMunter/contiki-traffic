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
 * Author: Oliver Schmidt <ol.sc@web.de>
 *
 * @(#)$Id: ethernet.c,v 1.2 2007/11/22 11:41:18 oliverschmidt Exp $
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <modload.h>

#include "contiki-net.h"
#include "ethernet-drv.h"

#include "ethernet.h"

struct {
  char                signature[4];
  struct uip_eth_addr ethernet_address;
  u8_t                *buffer;
  u16_t		      buffer_size;
  void __fastcall__   (* init)(u16_t reg);
  u16_t               (* poll)(void);
  void __fastcall__   (* send)(u16_t len);
  void                (* exit)(void);
} *module;

/*---------------------------------------------------------------------------*/
static void
error_exit(void)
{
  fprintf(stderr, "Press any key to continue ...\n");
  getchar();
  exit(EXIT_FAILURE);
}
/*---------------------------------------------------------------------------*/
void
ethernet_init(struct ethernet_config *config)
{
  static const char signature[4] = {0x65, 0x74, 0x68, 0x01};
  struct mod_ctrl module_control = {read};
  u8_t byte;

  module_control.callerdata = open(config->name, O_RDONLY);
  if(module_control.callerdata < 0) {
    fprintf(stderr, "%s: %s\n", config->name, strerror(errno));
    error_exit();
  }

  byte = mod_load(&module_control);
  if(byte != MLOAD_OK) {
    if(byte == MLOAD_ERR_MEM) {
      fprintf(stderr, "%s: %s\n", config->name, strerror(ENOMEM));
    } else {
      fprintf(stderr, "%s: %s %d\n", config->name, "Error", byte);
    }
    error_exit();
  }

  close(module_control.callerdata);
  module = module_control.module;

  for(byte = 0; byte < 4; byte++) {
    if(module->signature[byte] != signature[byte]) {
      fprintf(stderr, "%s: %s\n", config->name, "No ETH Driver");
      error_exit();
    }
  }

  module->buffer = uip_buf;
  module->buffer_size = UIP_BUFSIZE;
  module->init(config->addr);

  uip_setethaddr(module->ethernet_address);
}
/*---------------------------------------------------------------------------*/
u16_t
ethernet_poll(void)
{
  return module->poll();
}
/*---------------------------------------------------------------------------*/
void
ethernet_send(void)
{
  module->send(uip_len);
}
/*---------------------------------------------------------------------------*/
void
ethernet_exit(void)
{
  module->exit();

  mod_free(module);
}
/*---------------------------------------------------------------------------*/
