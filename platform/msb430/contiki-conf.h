#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#define HAVE_STDINT_H
#include "msp430def.h"

#define WITH_SDC			1

#define ENERGEST_CONF_ON		1

#define IRQ_PORT1			0x01
#define IRQ_PORT2			0x02
#define IRQ_ADC				0x03

// MSP430 Infomemory
#define INFOMEM_START			0x1000
#define INFOMEM_BLOCK_SIZE		128
#define INFOMEM_NODE_ID			0x0000 /* - 0x0004 */

#define CFS_SD_CONF_OFFSET		0x0000

#define CC_CONF_REGISTER_ARGS		1
#define CC_CONF_FUNCTION_POINTER_ARGS	1
#define CC_CONF_INLINE			inline
#define CC_CONF_VA_ARGS			1

#define CCIF
#define CLIF

/* Clear channel assessment timeout for sending with the CC1020 radio. (ms) */
#define CC1020_CONF_CCA_TIMEOUT		50

// Clock
typedef unsigned short clock_time_t;
#define CLOCK_CONF_SECOND		64
#define F_CPU				2457600uL // CPU target speed in Hz

#define BAUD2UBR(baud)			(F_CPU/(baud))

#include "ctk/ctk-vncarch.h"

#define LOG_CONF_ENABLED		0

/**
 * The statistics data type.
 *
 * This datatype determines how high the statistics counters are able
 * to count.
 */
typedef uint16_t uip_stats_t;

typedef int bool;
#define	TRUE				1
#define FALSE 				0

#define UIP_CONF_ICMP_DEST_UNREACH	1
#define UIP_CONF_DHCP_LIGHT
#define UIP_CONF_LLH_LEN		0
#define UIP_CONF_BUFFER_SIZE		116
#define UIP_CONF_RECEIVE_WINDOW		(UIP_CONF_BUFFER_SIZE - 40)
#define UIP_CONF_MAX_CONNECTIONS	4
#define UIP_CONF_MAX_LISTENPORTS	8
#define UIP_CONF_UDP_CONNS		8
#define UIP_CONF_FWCACHE_SIZE		20
#define UIP_CONF_BROADCAST		1
#define UIP_ARCH_IPCHKSUM		1
#define UIP_CONF_UDP_CHECKSUMS		1
#define UIP_CONF_PINGADDRCONF		0
#define UIP_CONF_LOGGING		0

#define LOADER_CONF_ARCH "loader/loader-arch.h"

#define ELFLOADER_CONF_DATAMEMORY_SIZE	100
#define ELFLOADER_CONF_TEXTMEMORY_SIZE	0x1000

/* LEDs ports MSB430 */
#define LEDS_PxDIR P5DIR
#define LEDS_PxOUT P5OUT
#define LEDS_CONF_RED			0x80
#define LEDS_CONF_GREEN			0x00
#define LEDS_CONF_YELLOW		0x00

#define SD_LED_READ_ON			(LEDS_PxOUT &=~LEDS_CONF_RED)
#define SD_LED_READ_OFF			(LEDS_PxOUT |= LEDS_CONF_RED)
#define SD_LED_WRITE_ON			SD_LED_READ_ON
#define SD_LED_WRITE_OFF		SD_LED_READ_OFF

// MSB430 SD Card driver
#define SD_READ_BYTE			0
#define	SD_READ_ANY			1
#define SD_INFO				0
#define SD_WRITE			1
#define SD_FIND_FILE			0
#define SD_ERASE			0			
#define	SD_CACHE			0
#define SPI_WRITE			SD_WRITE
#define SPI_DMA_READ			0
#define SPI_DMA_WRITE			0

#endif /* !CONTIKI_CONF_H */
