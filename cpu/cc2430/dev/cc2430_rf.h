/**
 * \file
 *         CC2430 RF driver header file
 * \author
 *         Zach Shelby <zach@sensinode.com>
 */

#ifndef __CC2430_RF_H__
#define __CC2430_RF_H__

#include "contiki.h"
#include "dev/radio.h"
#include "cc2430_sfr.h"
#include "dev/dma.h"

/* Constants */
typedef enum rf_address_mode_t
{
	RF_DECODER_NONE = 0,
	RF_DECODER_COORDINATOR,
	RF_SOFTACK_MONITOR,
	RF_MONITOR,
	RF_SOFTACK_CLIENT,
	RF_DECODER_ON
}rf_address_mode_t;
typedef enum
{
	MAC_NONE = 0,
	MAC_RECEIVE=1,
	MAC_ACK_RX=2,
	MAC_TIMER_ACK=3,
	MAC_TIMER_CCA=4,
	MAC_TRANSMIT=5,
	MAC_CONTROL=6,
	MAC_TIMER_NONE=7,
	MAC_LOOP=8,
	MAC_ED_SCAN=9,
	MAC_RSSI_CHECK=10,
	MAC_GW_DIS = 11
}mac_event_t;
/*CSP command set*/
#define SSTOP		0xDF
/*this is not a real command but a way of having rf_command
  wait until the script is done*/
#define SSTART		0xDE

#define SNOP		0xC0
#define STXCALN 0xC1
#define SRXON		0xC2
#define STXON		0xC3
#define STXONCCA	0xC4
#define SRFOFF		0xC5
#define SFLUSHRX	0xC6
#define SFLUSHTX	0xC7
#define SACK			0xC8
#define SACKPEND	0xC9

#define ISTXCALN 	0xE1
#define ISRXON		0xE2
#define ISTXON		0xE3
#define ISTXONCCA	0xE4
#define ISRFOFF		0xE5
#define ISFLUSHRX	0xE6
#define ISFLUSHTX	0xE7
#define ISACK		0xE8
#define ISACKPEND	0xE9

#define ISSTOP		0xFF
#define ISSTART		0xFE

#define MAC_IFS (1200/128)

#define CC2430_MAX_PACKET_LEN      127
#define CC2430_MIN_PACKET_LEN      4

extern const struct radio_driver cc2430_rf_driver;

/* radio_driver functions */
void cc2430_rf_set_receiver(void (* recv)(const struct radio_driver *));
int cc2430_rf_on(void);
int cc2430_rf_off(void);
int cc2430_rf_read(void *buf, unsigned short bufsize);
int cc2430_rf_send(const void *data, unsigned short len);

/* RF driver functions */
void cc2430_rf_init(void);
void cc2430_rf_command(uint8_t command);
int8_t cc2430_rf_channel_set(uint8_t channel);
int8_t cc2430_rf_power_set(uint8_t new_power);
int8_t cc2430_rf_rx_enable(void);
int8_t cc2430_rf_rx_disable(void);
int8_t cc2430_rf_tx_enable(void);
int8_t cc2430_rf_address_decoder_mode(rf_address_mode_t mode);
int8_t cc2430_rf_analyze_rssi(void);
int8_t cc2430_rf_cca_check(uint8_t backoff_count, uint8_t slotted);
void cc2430_rf_send_ack(uint8_t pending);

extern void cc2430_rf_ISR( void ) __interrupt (RF_VECTOR);
extern void cc2430_rf_error_ISR( void ) __interrupt (RFERR_VECTOR);

#ifdef HAVE_RF_DMA
void rf_dma_callback_isr(void);
#endif


#endif /* __CC2430_RF_H__ */
