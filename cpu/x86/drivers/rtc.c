/*
 * Copyright (C) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "drivers/rtc.h"
#include "helpers.h"
#include "interrupt.h"

#define RTC_INDEX_REGISTER   0x70
#define RTC_TARGET_REGISTER  0x71

static void (*user_callback)(void);

static void
rtc_handler()
{
  user_callback();

  /* Clear Register C otherwise interrupts will not happen again.
   * Register C is automatically cleared when it is read so we do
   * a dummy read to clear it.
   */
  outb(RTC_INDEX_REGISTER, 0x0C);
  inb(RTC_TARGET_REGISTER);

  /* Issue the End of Interrupt to PIC */
  outb(0xA0, 0x20);
  outb(0x20, 0x20);
}
/*---------------------------------------------------------------------------*/
/* Initialize the Real Time Clock.
 * @frequency: RTC has very specific values for frequency. They are: 2, 4, 8,
 *             16, 32, 64, 128, 256, 512, 1024, 2048, 4096, and 8192 Hz.
 *             value otherwise it will not work properly.
 * @callback:  This callback is called every time the RTC IRQ is raised.
 *             It is executed in interrupt context.
 */
void
rtc_init(rtc_frequency_t frequency, void (*callback)(void))
{
  uint8_t reg_a, reg_b;

  user_callback = callback;

  /* FIXME: Once we have a proper API to ask PIC what is the IRQ offset, we
   * should consider using it here.
   */
  SET_INTERRUPT_HANDLER(40, 0, rtc_handler);

  /* Select interrupt period to 7.8125 ms */
  outb(RTC_INDEX_REGISTER, 0x0A);
  reg_a = inb(RTC_TARGET_REGISTER);
  reg_a &= 0xF0;
  reg_a |= frequency;
  outb(RTC_INDEX_REGISTER, 0x0A);
  outb(RTC_TARGET_REGISTER, reg_a);

  /* Enable periodic interrupt */
  outb(RTC_INDEX_REGISTER, 0x0B);
  reg_b = inb(RTC_TARGET_REGISTER);
  outb(RTC_INDEX_REGISTER, 0x0B);
  outb(RTC_TARGET_REGISTER, reg_b | BIT(6));

  /* Unmask IRQ8 */
  outb(0xA1, 0xFE);
}
