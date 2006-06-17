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
 * @(#)$Id: cc2420.c,v 1.1 2006/06/17 22:41:16 adamdunkels Exp $
 */
/*
 * This code is almost device independent and should be possible to
 * port to the AVR.
 */

#include <stdio.h>
#include <string.h>

#include <io.h>
#include <signal.h>

#include "contiki.h"
#include "sys/clock.h"

#include "net/uip.h"
#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

#include "dev/leds.h"

#include "dev/spi.h"
#include "dev/cc2420.h"
#include "dev/cc2420_const.h"

PROCESS(cc2420_process, "CC2420 driver");
PROCESS(cc2420_retransmit_process, "CC2420 retransmit process");

int cc2420_resend(void);	/* Not yet exported. */

static void neigbour_update(u16_t mac, int retransmissions);

signed char cc2420_last_rssi;
u8_t cc2420_last_correlation;

static u8_t receive_on;

volatile u8_t cc2420_ack_received; /* Naive ACK management. */
static u8_t last_used_seq;
static u16_t last_correspondent;

/* Radio stuff in network byte order. */
static u16_t pan_id;

unsigned
cc2420_getreg(enum cc2420_register regname)
{
  unsigned reg;
  int s = splhigh();
  FASTSPI_GETREG(regname, reg);
  splx(s);
  return reg;
}

void
cc2420_setreg(enum cc2420_register regname, unsigned value)
{
  int s = splhigh();
  FASTSPI_SETREG(regname, value);
  splx(s);
}

void
cc2420_strobe(enum cc2420_register regname)
{
  int s = splhigh();
  FASTSPI_STROBE(regname);
  splx(s);
}

unsigned
cc2420_status(void)
{
  u8_t status;
  int s = splhigh();
  FASTSPI_UPD_STATUS(status);
  splx(s);
  return status;
}

#define PA_LEVEL 0x1F

unsigned
cc2420_pa_level(u16_t p)
{
  u16_t reg;

  /* 
   * All PA settings can be used, the ones given in the datasheet
   * correspond to 0, -1, -3, -5, -7, -10, -15, -25 dBm in the
   * reference design.
   */
  if (p > PA_LEVEL)
    p = PA_LEVEL;

  reg = cc2420_getreg(CC2420_TXCTRL);

  p |= reg & ~PA_LEVEL;

  cc2420_setreg(CC2420_TXCTRL, p);

  return reg & PA_LEVEL;
}

unsigned
cc2420_get_pa_level(void)
{
  u16_t reg;

  reg = cc2420_getreg(CC2420_TXCTRL);

  return reg & PA_LEVEL;
}

int
cc2420_get_current_rssi(void)
{
  u16_t reg;

  reg = cc2420_getreg(CC2420_RSSI);

  return (signed char) (reg & 0xff);
}

#define AUTOACK (1 << 4)
#define RXFIFO_PROTECTION (1 << 9)
#define CORR_THR(n) (((n) & 0x1f) << 6)
#define FIFOP_THR(n) ((n) & 0x7f)

void
cc2420_init(void)
{
  u16_t reg;
  {
    int s = splhigh();
    __cc2420_arch_init();		/* Initalize ports and SPI. */
    DISABLE_FIFOP_INT();
    FIFOP_INT_INIT();
    splx(s);
  }

  /* Turn on voltage regulator and reset. */
  SET_VREG_ACTIVE();
  //clock_delay(250); OK
  SET_RESET_ACTIVE();
  clock_delay(127);
  SET_RESET_INACTIVE();
  //clock_delay(125); OK


  /* Turn on the crystal oscillator. */
  cc2420_strobe(CC2420_SXOSCON);

  /* Turn on automatic packet acknowledgment. */
  reg = cc2420_getreg(CC2420_MDMCTRL0);
  reg |= AUTOACK;
  cc2420_setreg(CC2420_MDMCTRL0, reg);

  /* Set the correlation threshold = 20. */
  cc2420_setreg(CC2420_MDMCTRL1, CORR_THR(20));

  /* Set the FIFOP threshold to maximum. */
  cc2420_setreg(CC2420_IOCFG0, FIFOP_THR(127));

  /* Turn off "Security enable" (page 32). */
  reg = cc2420_getreg(CC2420_SECCTRL0);
  reg &= ~RXFIFO_PROTECTION;
  cc2420_setreg(CC2420_SECCTRL0, reg);

  cc2420_set_chan_pan_addr(11, 0xffff, 0x0000, NULL);
}

int
cc2420_send_data_ack(u16_t mac)
{
  struct hdr_802_15 h;

  h.len = MAC_HDR_LEN + 2; /* Including footer[2]. */
  h.fc0 = FC0_TYPE_DATA | FC0_INTRA_PAN;
  h.fc1 = FC1_DST_16 | FC1_SRC_16;

  h.src = uip_hostaddr[1];
  h.dst = mac;

  cc2420_send(&h, 10, NULL, 0);

  return 0;
}

int
cc2420_send(struct hdr_802_15 *hdr, u8_t hdr_len,
	    const u8_t *payload, u8_t payload_len)
{
  u8_t spiStatusByte;
  int s;

  /* struct hdr_802_15::len shall *not* be counted, thus the -1.
   * 2 == sizeof(footer).
   */
  if (((hdr_len - 1) + payload_len + 2) > MAX_PACKET_LEN)
    return -1;

  /* This code uses the CC2420 CCA (Clear Channel Assessment) to
   * implement Carrier Sense Multiple Access with Collision Avoidance
   * (CSMA-CA) and requires the receiver to be enabled and ready.
   */
  if (!receive_on)
    return -2;

  /* Wait for previous transmission to finish and RSSI. */
  do {
    spiStatusByte = cc2420_status();
    if (!(spiStatusByte & BV(CC2420_RSSI_VALID))) /* RSSI needed by CCA */
      continue;
  } while (spiStatusByte & BV(CC2420_TX_ACTIVE));

  hdr->dst_pan = pan_id;	/* Not at fixed position! xxx/bg */
  last_correspondent = hdr->dst; /* Not dst either. */
  last_used_seq++;
  hdr->seq = last_used_seq;
  cc2420_ack_received = 0;

  /* Write packet to TX FIFO, appending FCS if AUTOCRC is enabled. */
  cc2420_strobe(CC2420_SFLUSHTX); /* Cancel send that never started. */
  s = splhigh();
  FASTSPI_WRITE_FIFO(hdr, hdr_len);
  FASTSPI_WRITE_FIFO(payload, payload_len);
  splx(s);

  if (hdr->dst != 0xffff)
    process_post(&cc2420_retransmit_process,
		 PROCESS_EVENT_MSG,
		 (void *)(unsigned)last_used_seq);
  return cc2420_resend();	/* Send stuff from FIFO. */
}

int
cc2420_resend(void)
{
  int i;
  
  /* Request packet to be sent using CSMA-CA. */
  cc2420_strobe(CC2420_STXONCCA);

  /* The RX FIFO can only hold one packet! Make sure to not overrun
   * FIFO by waiting for transmission to start here and synchronizing
   * with the CC2420_TX_ACTIVE check in cc2420_send.
   *
   * If RX FIFO is full or CSMA-CA never says ok we will have to
   * terminate the loop below before SFD_IS_1!
   */
  for (i = 0; i < 1000; i++)
    if (SFD_IS_1)
      return 0;			/* Transmission has started. */

  /*
   * In the exceptional case that transmission never occurred we try
   * again but never verify if the transmission ever starts. If a new
   * call to cc2420_send() happens in a near future this transmission
   * will be permanently canceled.
   */
  cc2420_strobe(CC2420_STXONCCA);
  return 0;
}

void
cc2420_off(void)
{
  u8_t spiStatusByte;

  if (receive_on == 0)
    return;
  receive_on = 0;
  /* Wait for transmission to end before turning radio off. */
  do {
    spiStatusByte = cc2420_status();
  } while (spiStatusByte & BV(CC2420_TX_ACTIVE));

  cc2420_strobe(CC2420_SRFOFF);
  DISABLE_FIFOP_INT();
}

void
cc2420_on(void)
{
  if (receive_on)
    return;
  receive_on = 1;

  cc2420_strobe(CC2420_SRXON);
  cc2420_strobe(CC2420_SFLUSHRX);
  ENABLE_FIFOP_INT();
}

void
cc2420_set_chan_pan_addr(unsigned channel, unsigned pan,
			 unsigned addr, const u8_t *ieee_addr)
{
  /*
   * Subtract the base channel (11), multiply by 5, which is the
   * channel spacing. 357 is 2405-2048 and 0x4000 is LOCK_THR = 1.
   */
  u8_t spiStatusByte;
  u16_t f = channel;
  int s;
        
  f = 5*(f - 11) + 357 + 0x4000;
  /*
   * Writing RAM requires crystal oscillator to be stable.
   */
  do {
    spiStatusByte = cc2420_status();
  } while (!(spiStatusByte & (BV(CC2420_XOSC16M_STABLE))));

  pan_id = pan;
  cc2420_setreg(CC2420_FSCTRL, f);
  s = splhigh();
  FASTSPI_WRITE_RAM_LE(&pan, CC2420RAM_PANID, 2, f);
  FASTSPI_WRITE_RAM_LE(&addr, CC2420RAM_SHORTADDR, 2, f);
  if (ieee_addr != NULL)
    FASTSPI_WRITE_RAM_LE(ieee_addr, CC2420RAM_IEEEADDR, 8, f);
  splx(s);
}

static volatile u8_t rx_fifo_remaining_bytes;
static struct hdr_802_15 h;
    
/*
 * Interrupt either leaves frame intact in FIFO or reads *only* the
 * MAC header and sets rx_fifo_remaining_bytes.
 *
 * In order to quickly empty the FIFO ack processing is done at
 * interrupt priority rather than poll priority.
 */
int
__cc2420_intr(void)
{
  u8_t length;
  const u8_t *const ack_footer = (u8_t *)&h.dst_pan;

  CLEAR_FIFOP_INT();
    
  if (spi_busy || rx_fifo_remaining_bytes > 0) {
    /* SPI bus hardware is currently used elsewhere (UART0 or I2C bus)
     * or we already have a packet in the works and will have to defer
     * interrupt processing of this packet in a fake interrupt.
     */
    process_poll(&cc2420_process);
    return 1;
  }

  FASTSPI_READ_FIFO_BYTE(length);
  if (length > MAX_PACKET_LEN) {
    /* Oops, we must be out of sync. */
    //printf("__cc2420_intr out of sync\n");
    FASTSPI_STROBE(CC2420_SFLUSHRX);
    FASTSPI_STROBE(CC2420_SFLUSHRX);
    return 0;
  }

  h.len = length;

  if (length < ACK_PACKET_LEN) {
    FASTSPI_READ_FIFO_GARBAGE(length); /* Rubbish */
    return 0;
  }

  FASTSPI_READ_FIFO_NO_WAIT(&h.fc0, 5); /* fc0, fc1, seq, dst_pan */

  /* Is this an ACK packet? */
  if (length == ACK_PACKET_LEN && (h.fc0 & FC0_TYPE_MASK) == FC0_TYPE_ACK) {
    if (ack_footer[1] & FOOTER1_CRC_OK) {
      if (h.seq == last_used_seq) { /* Matching ACK number? */
	cc2420_ack_received = 1;
	process_poll(&cc2420_retransmit_process);
#if 0
	cc2420_last_rssi = ack_footer[0];
	cc2420_last_correlation = ack_footer[1] & FOOTER1_CORRELATION;
#endif
      }
    }
    return 1;
  }

  if (length < (MAC_HDR_LEN + 2)) {
    FASTSPI_READ_FIFO_GARBAGE(length - 5);
    return 0;
  }

  FASTSPI_READ_FIFO_NO_WAIT(&h.dst, 4); /* dst and src */

  /* The payload and footer is now left in the RX FIFO and will be
   * picked up asynchronously at poll priority in the cc2420_process
   * below.
   */
  rx_fifo_remaining_bytes = length - MAC_HDR_LEN;
  process_poll(&cc2420_process);
  return 1;
}

PROCESS_THREAD(cc2420_process, ev, data)
{
  PROCESS_BEGIN();

  process_start(&cc2420_retransmit_process, NULL);

  while (1) {
    unsigned len;
    int s;

    PROCESS_YIELD();

    len = rx_fifo_remaining_bytes;
    if (len > 0) {
      /* Read payload and two bytes of footer */
      if ((len - 2) > (UIP_BUFSIZE - UIP_LLH_LEN) || len < 2) {
	printf("cc2420_process too big len=%d\n", len);
	s = splhigh();
	FASTSPI_READ_FIFO_GARBAGE(len);
	rx_fifo_remaining_bytes = 0; /* RX FIFO emptied! */
	splx(s);
	len = 0;
      } else {
	u8_t footer[2];
	uip_len = 0;
	s = splhigh();
	FASTSPI_READ_FIFO_NO_WAIT(&uip_buf[UIP_LLH_LEN], len - 2);
	FASTSPI_READ_FIFO_NO_WAIT(footer, 2);
	rx_fifo_remaining_bytes = 0; /* RX FIFO emptied! */
	splx(s);
	if (footer[1] & FOOTER1_CRC_OK) {
	  cc2420_last_rssi = footer[0];
	  cc2420_last_correlation = footer[1] & FOOTER1_CORRELATION;
	  if ((h.fc0 & FC0_TYPE_MASK) == FC0_TYPE_DATA)
	    uip_len = len - 2;
	}
      }
    }

    /* Clean up in case of FIFO overflow!  This happens for every full
     * length frame and is signaled by FIFOP = 1 and FIFO = 0.
     */
    if (FIFOP_IS_1 && !FIFO_IS_1) {
      cc2420_strobe(CC2420_SFLUSHRX);
      cc2420_strobe(CC2420_SFLUSHRX);
    }

    if (FIFOP_IS_1) {
      s = splhigh();
      __cc2420_intr();		/* Fake interrupt! */
      splx(s);
    }

    if (len == 2) {		/* A DATA ACK packet. */
      if (last_correspondent == h.src)
	cc2420_ack_received = 1;
      neigbour_update(h.src, 0);
    } else if (len > 2 && uip_len > 0
       && uip_len == (((u16_t)(BUF->len[0]) << 8) + BUF->len[1])) {
      /*
       * If we are the unique receiver send DATA ACK.
       */
      if (h.dst == 0xffff
	  && BUF->destipaddr[0] == uip_hostaddr[0]
	  && BUF->destipaddr[1] == uip_hostaddr[1])
	cc2420_send_data_ack(h.src);
      leds_toggle(LEDS_GREEN);
      tcpip_input();
      leds_toggle(LEDS_GREEN);
    }
  }

  PROCESS_END();
}

unsigned    neigbour_find(u16_t mac);

/* Must be atleast 2 ticks and larger than 4ms. */
#define RETRANSMIT_TIMEOUT  2	/* 31.25ms @ 64Hz */
#define MAX_RETRANSMISSIONS 3

PROCESS_THREAD(cc2420_retransmit_process, ev, data)
{
  static u8_t seq, n;
  static struct etimer etimer;

  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_UNTIL(ev == PROCESS_EVENT_MSG);
    seq = (unsigned)data;

    n = 0;
    do {
      etimer_set(&etimer, RETRANSMIT_TIMEOUT);
      PROCESS_WAIT_UNTIL(etimer_expired(&etimer) || ev == PROCESS_EVENT_POLL);
      if (ev == PROCESS_EVENT_POLL) {
	etimer_stop(&etimer);
	break;
      } else if (seq != last_used_seq)
	break;			/* Transmitting different packet. */
      else if (n < MAX_RETRANSMISSIONS) {
	cc2420_resend();
	n++;
      } else {
	break;
      }
    } while (1);
    neigbour_update(last_correspondent, n);
#if 0
#define CORRELATION_2_X(c) (((c) < 48) ? 0 : ((c) - 48))
    printf("%04x %2d %2d %2u %u\n",
	   last_correspondent, n,
	   RSSI_2_ED(cc2420_last_rssi),
	   CORRELATION_2_X(cc2420_last_correlation),
	   clock_time());
#endif
  }

  PROCESS_END();
}

/*
 * Retransmissions are negexp(alfa=0.5) weighted and stored as 4-bit
 * fixnums with 2 binals (binary decimals).
 */
#define SCALE_RETRANS              4
#define SCALE_RETRANS_THRESHOLD (3*4)
#define MAX_SCALE_RETRANS         15

/*
 * Expiration timestamps are 4-bits wide, in units of 4 seconds, and
 * relative to cc2420_check_remote::toff.
 */
#define SCALE_DIV_EXPIRE    4
#define MAX_EXPIRE         15
#define AGE_INTERVAL        5	/* 20 seconds */

struct cc2420_neigbour {
  unsigned mac:16, nretrans:4, expire:4; /* expiration time */
#if 0
  unsigned rssi:6, correlation:6;
#endif
};

#define NNEIGBOURS 16
static struct cc2420_neigbour neigbours[NNEIGBOURS];

/*
 * Double hash into 3 different positions using a constand step. If we
 * don't find a match, return a pointer to the oldest entry and use
 * this position for insertion.
 */
static struct cc2420_neigbour *
lookup(unsigned mac)
{
  unsigned h = (mac + (mac>>8)) % NNEIGBOURS;
#define next(h) (h += step, (h >= NNEIGBOURS) ? (h - NNEIGBOURS) : h)

  if (neigbours[h].mac == mac)	/* FOUND1 */
    return &neigbours[h];
  else {
    unsigned minexp = h;
    const unsigned step = ((mac>>9)&0x3) + 1;

    h = next(h);
    if (neigbours[h].mac == mac) /* FOUND2 */
      return &neigbours[h];
    else {
      if (neigbours[h].expire < neigbours[minexp].expire) minexp = h;
      h = next(h);
      if (neigbours[h].mac == mac) /* FOUND3 */
	return &neigbours[h];
      else {
	if (neigbours[h].expire < neigbours[minexp].expire) minexp = h;
	return &neigbours[minexp];
      }
    }
  }
}

static void
neigbour_update(u16_t mac, int nretrans)
{
  struct cc2420_neigbour *t;

  /* Always scale nretrans by constant factor. */
  if (nretrans == MAX_RETRANSMISSIONS)
    nretrans = MAX_SCALE_RETRANS;
  else
    nretrans *= SCALE_RETRANS;	/* xxx/bg overflow! */

  t = lookup(mac);
  if (t->mac != mac) {
    t->mac = mac;
    t->nretrans = nretrans;
  } else {
    if ((t->nretrans + nretrans)/2 > MAX_SCALE_RETRANS)
      t->nretrans = MAX_SCALE_RETRANS;
    else
      t->nretrans = (t->nretrans + nretrans)/2;
  }
  t->expire = MAX_EXPIRE;
  //printf("+ %04x %d/4 %d\n", t->mac, t->nretrans, t->expire);
  return;
}

void
cc2420_recv_ok(uip_ipaddr_t *from)
{
  neigbour_update((*from)[1], 0);
}

/*
 * +1: remote
 *  0: local
 * -1: unknown
 */
int
cc2420_check_remote(u16_t mac)
{
  struct cc2420_neigbour *t;

  /*
   * Age neigbour table every 5*SCALE_DIV_EXPIRE=20 seconds.
   */
  static clock_time_t toff;
  unsigned now = ((clock_time() - toff)/CLOCK_SECOND)/SCALE_DIV_EXPIRE;
  if (now >= AGE_INTERVAL) {
    unsigned i;

    //printf("aging\n");
    for (i = 0; i < NNEIGBOURS; i++)
      if (neigbours[i].expire >= now)
	neigbours[i].expire -= now;
      else
	neigbours[i].mac = 0xffff; /* expired! */
    toff = clock_time();
  }

  t = lookup(mac);
  //printf("  %04x %d/4 %d\n", t->mac, t->nretrans, t->expire);
  if (t->mac != mac)
    return -1;			/* unknown */
  else if (t->nretrans >= SCALE_RETRANS_THRESHOLD)
    return +1;			/* remote */
  else
    return  0;			/* local */
}
