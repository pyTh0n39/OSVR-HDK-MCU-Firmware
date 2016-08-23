/**
 * \file
 *
 * \brief TWIM Configuration File for AVR XMEGA.
 *
 * Copyright (c) 2011 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#ifndef _CONF_TWIM_H_
#define _CONF_TWIM_H_

#define CONF_TWIM_INTLVL        TWI_MASTER_INTLVL_MED_gc
#define CONF_PMIC_INTLVL        PMIC_MEDLVLEN_bm

// configuration that is specific to the Sensics board

#include "GlobalOptions.h"

#ifdef BNO070
	#define TWI_BNO070_PORT			(&TWIC)
#endif

#ifdef SVR_HAVE_NXP1
	#define TWI_NXP1_PORT			(&TWIE) // j4 on ATMega Xplained for A3BU
#endif

#ifdef SVR_HAVE_TMDS422
    #define TWI_TMDS422_PORT        (&TWIC)
#endif

#ifdef SVR_HAVE_NXP2
    #define TWI_NXP2_PORT			(&TWIC) // same port as the TMDS one
#endif

#ifdef SVR_HAVE_TOSHIBA_TC358870
	/// @todo Defined, but not used anywhere because all interaction was hidden in libhdk2

#endif

#endif // _CONF_TWIM_H_
