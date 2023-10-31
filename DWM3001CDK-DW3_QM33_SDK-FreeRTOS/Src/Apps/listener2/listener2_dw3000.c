/**
 * @file      listener2_dw3000.c
 * 
 * @brief     Decawave Application level
 *             collection of TWR bare-metal functions for a Listener
 *
 * @author    Decawave Applications
 *
 * @attention Copyright (c) 2021 - 2022, Qorvo US, Inc.
 * All rights reserved
 * Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this
 *  list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 3. You may only use this software, with or without any modification, with an
 *  integrated circuit developed by Qorvo US, Inc. or any of its affiliates
 *  (collectively, "Qorvo"), or any module that contains such integrated circuit.
 * 4. You may not reverse engineer, disassemble, decompile, decode, adapt, or
 *  otherwise attempt to derive or gain access to the source code to any software
 *  distributed under this license in binary or object code form, in whole or in
 *  part.
 * 5. You may not use any Qorvo name, trademarks, service marks, trade dress,
 *  logos, trade names, or other symbols or insignia identifying the source of
 *  Qorvo's products or services, or the names of any of Qorvo's developers to
 *  endorse or promote products derived from this software without specific prior
 *  written permission from Qorvo US, Inc. You must not call products derived from
 *  this software "Qorvo", you must not have "Qorvo" appear in their name, without
 *  the prior permission from Qorvo US, Inc.
 * 6. Qorvo may publish revised or new version of this license from time to time.
 *  No one other than Qorvo US, Inc. has the right to modify the terms applicable
 *  to the software provided under this license.
 * THIS SOFTWARE IS PROVIDED BY QORVO US, INC. "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. NEITHER
 *  QORVO, NOR ANY PERSON ASSOCIATED WITH QORVO MAKES ANY WARRANTY OR
 *  REPRESENTATION WITH RESPECT TO THE COMPLETENESS, SECURITY, RELIABILITY, OR
 *  ACCURACY OF THE SOFTWARE, THAT IT IS ERROR FREE OR THAT ANY DEFECTS WILL BE
 *  CORRECTED, OR THAT THE SOFTWARE WILL OTHERWISE MEET YOUR NEEDS OR EXPECTATIONS.
 * IN NO EVENT SHALL QORVO OR ANYBODY ASSOCIATED WITH QORVO BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *
 */

/* Includes */
#include <stdint.h>
#include <math.h>

#include "listener2.h"
#include "deca_device_api.h"
#include "driver_app_config.h"
#include "rf_tuning_config.h"

#define RX_CODE_THRESHOLD  8      // For 64 MHz PRF the RX code is 9.
#define ALPHA_PRF_16       113.8  // Constant A for PRF of 16 MHz. See User Manual for more information.
#define ALPHA_PRF_64       120.7  // Constant A for PRF of 64 MHz. See User Manual for more information.
#define LOG_CONSTANT_C0    63.2   // 10log10(2^21) = 63.2    // See User Manual for more information.
#define LOG_CONSTANT_D0_E0 51.175 // 10log10(2^17) = 51.175  // See User Manual for more information.


void listener2_rssi_cal(int *rsl100, int *fsl100)
{
    dwt_nlos_alldiag_t all_diag;
    uint8_t D;
    dwt_config_t *dwt_config = get_dwt_config();

    // All float variables used for recording different diagnostic results and probability.
    float ip_alpha, log_constant = 0;
    float ip_f1, ip_f2, ip_f3, ip_n, ip_cp, ip_rsl, ip_fsl;

    uint32_t dev_id = dwt_readdevid();

    if ((dev_id == (uint32_t)DWT_DW3000_DEV_ID) || (dev_id == (uint32_t)DWT_DW3000_PDOA_DEV_ID))
    {
        log_constant = LOG_CONSTANT_C0;
    }
    else
    {
        log_constant = LOG_CONSTANT_D0_E0;
    }

    // Select IPATOV to read Ipatov diagnostic registers from API function dwt_nlos_alldiag()
    all_diag.diag_type = IPATOV;
    dwt_nlos_alldiag(&all_diag);
    ip_alpha = (dwt_config->rxCode > RX_CODE_THRESHOLD) ? (-(ALPHA_PRF_64 + 1)) : -(ALPHA_PRF_16);
    // ip_alpha = (config->rxCode > RX_CODE_THRESHOLD) ? (-(ALPHA_PRF_64 + 1)) : -(ALPHA_PRF_16);
    ip_n = all_diag.accumCount; // The number of preamble symbols accumulated
    ip_f1 = all_diag.F1 / 4;    // The First Path Amplitude (point 1) magnitude value (it has 2 fractional bits),
    ip_f2 = all_diag.F2 / 4;    // The First Path Amplitude (point 2) magnitude value (it has 2 fractional bits),
    ip_f3 = all_diag.F3 / 4;    // The First Path Amplitude (point 3) magnitude value (it has 2 fractional bits),
    ip_cp = all_diag.cir_power;

    D = all_diag.D * 6;

    // For IPATOV
    ip_n *= ip_n;
    ip_f1 *= ip_f1;
    ip_f2 *= ip_f2;
    ip_f3 *= ip_f3;

    // For the CIR Ipatov.
    ip_rsl = 10 * log10((float)ip_cp / ip_n) + ip_alpha + log_constant + D;
    ip_fsl = 10 * log10(((ip_f1 + ip_f2 + ip_f3) / ip_n)) + ip_alpha + D;
    if ((ip_rsl < -120) || (ip_rsl > 0))
    {
        ip_rsl = 0;
    }
    if ((ip_fsl < -120) || (ip_fsl > 0))
    {
        ip_fsl = 0;
    }
    *rsl100 = (int)(ip_rsl * 100);
    *fsl100 = (int)(ip_fsl * 100);
}

void listener2_readrxtimestamp(uint8_t *timestamp)
{
    dwt_readrxtimestamp(timestamp);
}

int listener2_readstsquality(int16_t *rxStsQualityIndex)
{
    return dwt_readstsquality(rxStsQualityIndex);
}

void listener2_configure_uwb(dwt_cb_t cbRxOk, dwt_cb_t cbRxTo, dwt_cb_t cbRxErr)
{
    rf_tuning_t *rf_tuning = get_rf_tuning_config();
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK); /**< DEBUG I/O 2&3 : configure the GPIOs which control the LEDs on HW */
    dwt_setlnapamode(DWT_PA_ENABLE | DWT_LNA_ENABLE);   /**< DEBUG I/O 5&6 : configure TX/RX states to output on GPIOs */

    dwt_setcallbacks(NULL, cbRxOk, cbRxTo, cbRxErr, NULL, NULL, NULL);

    dwt_setinterrupt(DWT_INT_TXFRS_BIT_MASK | DWT_INT_RXFCG_BIT_MASK
                         | (DWT_INT_ARFE_BIT_MASK | DWT_INT_RXFSL_BIT_MASK | DWT_INT_RXSTO_BIT_MASK | DWT_INT_RXPHE_BIT_MASK
                            | DWT_INT_RXFCE_BIT_MASK | DWT_INT_RXFTO_BIT_MASK /*| SYS_STATUS_RXPTO_BIT_MASK*/),
                     0, 2);


    dwt_configciadiag(DW_CIA_DIAG_LOG_ALL); /* DW3000 */

    // configure STS KEY/IV
    sts_config_t *sts_config = get_sts_config();
    dwt_configurestskey(&sts_config->stsKey);
    dwt_configurestsiv(&sts_config->stsIv);
    // load the configured KEY/IV values
    dwt_configurestsloadiv();

    dwt_configeventcounters(1);

    /*
     * The dwt_initialize will read the default XTAL TRIM from the OTP or use the DEFAULT_XTAL_TRIM.
     * In this case we would apply the user-configured value.
     *
     * Bit 0x80 can be used to overwrite the OTP settings if any.
     * */
    if ((dwt_getxtaltrim() == DEFAULT_XTAL_TRIM) || (rf_tuning->xtalTrim & ~XTAL_TRIM_BIT_MASK))
    {
        dwt_setxtaltrim(rf_tuning->xtalTrim & XTAL_TRIM_BIT_MASK);
    }
}

void listener2_deinit(void)
{
    // DW_IRQ is disabled: safe to cancel all user call-backs
    dwt_setcallbacks(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}
