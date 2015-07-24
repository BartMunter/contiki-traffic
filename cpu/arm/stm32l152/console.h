/**
 ******************************************************************************
 * @file    console.h
 * @author  AST
 * @version V1.0.0
 * @date    26-Aug-2014
 * @brief   This file provides implementation of standard input/output
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
 

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup X_NUCLEO_IKC01A1_Demonstration
  * @{
  */ 

/** @defgroup X_NUCLEO_IKC01A1_Demonstration_Console_Utilities
 * @{
 */
 

/**
 * @brief  Initialises Nucleo UART port for user IO
 * @param  None
 * @retval 0
 */


#if defined (__IAR_SYSTEMS_ICC__)

size_t __write(int Handle, const unsigned char * Buf, size_t Bufsize);
size_t __read(int Handle, unsigned char *Buf, size_t Bufsize);


#elif defined (__CC_ARM)

/**
 * @brief fputc call for standard output implementation
 * @param ch Character to print
 * @param f File pointer
 * @retval Character printed
 */
int fputc(int ch, FILE *f);

/** @brief fgetc call for standard input implementation
 * @param f File pointer
 * @retval Character acquired from standard input
 */
int fgetc(FILE *f);

#elif defined (__GNUC__)

/** @brief putchar call for standard output implementation
 * @param ch Character to print
 * @retval Character printed
 */
int __io_putchar(int ch);

/** @brief getchar call for standard input implementation
 * @param None
 * @retval Character acquired from standard input
 */
int __io_getchar(void);

#else
#error "Toolchain not supported"
#endif
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
