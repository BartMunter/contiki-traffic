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
 * $Id: cfs-eeprom.c,v 1.1 2006/06/17 22:41:15 adamdunkels Exp $
 */
#include "contiki.h"

#include "cfs/cfs.h"
#include "cfs/cfs-service.h"
#include "dev/eeprom.h"

struct filestate {
  int flag;
#define FLAG_FILE_CLOSED 0
#define FLAG_FILE_OPEN   1
  eeprom_addr_t fileptr;
};

static struct filestate file;

#ifdef CFS_EEPROM_CONF_OFFSET
#define CFS_EEPROM_OFFSET CFS_EEPROM_CONF_OFFSET
#else
#define CFS_EEPROM_OFFSET 0
#endif

/*---------------------------------------------------------------------------*/
static int
s_open(const char *n, int f)
{
  if(file.flag == FLAG_FILE_CLOSED) {
    file.flag = FLAG_FILE_OPEN;
    file.fileptr = 0;
    return 1;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static void
s_close(int f)
{
  file.flag = FLAG_FILE_CLOSED;
}
/*---------------------------------------------------------------------------*/
static int
s_read(int f, char *buf, unsigned int len)
{
  if(f == 1) {
    eeprom_read(CFS_EEPROM_OFFSET + file.fileptr, buf, len);
    file.fileptr += len;
    return len;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
s_write(int f, char *buf, unsigned int len)
{
  if(f == 1) {
    eeprom_write(CFS_EEPROM_OFFSET + file.fileptr, buf, len);
    file.fileptr += len;
    return len;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
s_seek(int f, unsigned int o)
{
  if(f == 1) {
    file.fileptr = o;
    return o;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
s_opendir(struct cfs_dir *p, const char *n)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
s_readdir(struct cfs_dir *p, struct cfs_dirent *e)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
s_closedir(struct cfs_dir *p)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
/*
 * Service registration code follows.
 */
SERVICE(cfs_eeprom_service, cfs_service,
{ s_open, s_close, s_read, s_write, s_seek,
    s_opendir, s_readdir, s_closedir });

PROCESS(cfs_eeprom_process, "CFS EEPROM service");

PROCESS_THREAD(cfs_eeprom_process, ev, data)
{
  PROCESS_BEGIN();

  SERVICE_REGISTER(cfs_eeprom_service);

  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_SERVICE_REMOVED ||
			   ev == PROCESS_EVENT_EXIT);

  SERVICE_REMOVE(cfs_eeprom_service);
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
