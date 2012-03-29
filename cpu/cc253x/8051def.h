/*
 * \file
 *	This file contains a set of configuration for using SDCC as a compiler.
 *	Modified from z80 port for cc2430 port.
 *
 * \author
 *	 Takahide Matsutsuka <markn@markn.org> (Original)
 *	 George Oikonomou - <oikonomou@users.sourceforge.net>
 *	   (updates for the cc2530 ports)
 */

#ifndef __8051_DEF_H__
#define __8051_DEF_H__

#include <stdint.h>

/*
 * lint - style defines to help syntax parsers with sdcc-specific 8051 code
 * They don't interfere with actual compilation
 */
#if !defined(__SDCC_mcs51) && !defined(SDCC_mcs51)
#define __data
#define __xdata
#define __code
#define __bit bool
#define __sfr volatile unsigned char
#define __sbit volatile bool
#define __critical
#define __at(x)
#define __using(x)
#define __interrupt(x)
#define __naked
#endif

#define CC_CONF_FUNCTION_POINTER_ARGS	1
#define CC_CONF_FASTCALL
#define CC_CONF_VA_ARGS		1
#define CC_CONF_UNSIGNED_CHAR_BUGS	0
#define CC_CONF_REGISTER_ARGS		0
#define CC_CONF_FUNCTION_POINTER_KEYWORD __reentrant

/* Generic types. */
typedef unsigned short uip_stats_t;

/* Compiler configurations */
#define CCIF
#define CLIF

/* Single asm instruction without messing up syntax highlighting */
#if defined SDCC_mcs51
#define ASM(x) __asm \
  x \
  __endasm
#else
#define ASM(x)
#endif

/* Critical section management */
#define DISABLE_INTERRUPTS()  do {EA = 0;} while(0)
#define ENABLE_INTERRUPTS()   do {EA = 1;} while(0)

/* Macro for a soft reset. In many respects better than H/W reboot via W/D */
#define SOFT_RESET() {((void (__code *) (void)) 0x0000) ();}

/* We don't provide architecture-specific checksum calculations */
#define UIP_ARCH_ADD32    0
#define UIP_ARCH_CHKSUM	  0

#define CC_CONF_ASSIGN_AGGREGATE(dest, src)	\
    memcpy(dest, src, sizeof(*dest))

#define uip_ipaddr_copy(dest, src)		\
    memcpy(dest, src, sizeof(*dest))

#endif /* __8051_DEF_H__ */
