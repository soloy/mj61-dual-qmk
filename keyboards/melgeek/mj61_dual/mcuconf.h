/*
  ChibiOS - Copyright (C) 2020 Alex Lewontin

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef _MCUCONF_H_
#define _MCUCONF_H_

/*
 * Clock setup copied from ducky/one2mini/1861st (12 MHz HSE, PLL, HCLK 72 MHz).
 * TODO: verify the MJ61 DUAL board has a 12 MHz crystal on the NUC123.
 */
#define NUC123_HSE_ENABLED TRUE
#define NUC123_PLL_ENABLED TRUE
#define NUC123_PLLSRC NUC123_PLLSRC_HSE
#define NUC123_HCLKSRC NUC123_HCLKSRC_PLL
#define NUC123_HCLKDIV 2
#define NUC123_PLL_NF 144
#define NUC123_USB_USE_USB0 TRUE
#define NUC123_USB_USE_USB1 TRUE

/* IS31FL3741 on I2C0 (PF2/PF3), see config.h. */
#define NUC123_I2C_USE_I2C0 TRUE
/* 24Cxx EEPROM on I2C1 (PA10/PA11), see eeprom_i2c1.c. */
#define NUC123_I2C_USE_I2C1 TRUE

/* UART1 (PB4/PB5) to the nRF52, see mj61_bt.c. */
#ifdef BLUETOOTH_ENABLE
#    undef NUC123_SERIAL_USE_UART1
#    define NUC123_SERIAL_USE_UART1 TRUE
#endif

#define NUC123_MCUCONF

#endif /* _MCUCONF_H_ */
