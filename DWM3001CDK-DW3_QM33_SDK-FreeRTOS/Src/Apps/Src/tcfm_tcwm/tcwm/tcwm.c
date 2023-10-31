/**
 * @file      tcwm.c
 *
 * @brief     Process to test continuous wave mode
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

#include "tcwm.h"

#include "app.h"
#include "deca_device_api.h"
#include "appConfig.h"
#include "driver_app_config.h"
#include "rf_tuning_config.h"
#include "HAL_uwb.h"
#include "HAL_error.h"

/* IMPLEMETATION */

/*
 * @brief     init function initialises all run-time environment allocated by the process
 *             it will be executed once
 * */
void tcwm_process_init(void)
{
    tcXm_configure_test_mode();

    dwt_configcwmode();
}


/*
 * @brief     run function implements continuous process functionality
 * */
void tcwm_process_run(void)
{
    /*do nothing*/
}


/*
 * @brief     stop function implements stop functionality if any
 *             which will be executed on reception of Stop command
 * */
void tcwm_process_terminate(void)
{
    hal_uwb.stop_all_uwb();
}

/*
 * @brief     configure channel parameters and tx spectrum parameters
 * */

void tcXm_configure_test_mode(void)
{
    int result;

    result = dwt_initialise(0);

    if (DWT_SUCCESS != result)
    {
        error_handler(1, _ERR_INIT);
    }

    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK); /* For debug - to see TX LED light up when in this mode */

#ifndef CONFIG_PEG_UWB
    dwt_setlnapamode(DWT_PA_ENABLE);                    /* For debug - to see TX state output and be able to measure the packet length */
#endif

    dwt_config_t *dwt_config = get_dwt_config();
    if (dwt_configure(dwt_config)) /**< Configure the Physical Channel parameters (PLEN, PRF, etc) */
    {
        error_handler(1, _ERR_INIT);
    }

    /* configure power */
    dwt_txconfig_t *txConfig = get_dwt_txconfig();
    dwt_configuretxrf(txConfig);

    /*
     * The dwt_initialize will read the default XTAL TRIM from the OTP or use the DEFAULT_XTAL_TRIM.
     * In this case we would apply the user-configured value.
     *
     * The bit 0x80 can be used to overwrite the OTP settings if any.
     * */
    rf_tuning_t *rf_tuning = get_rf_tuning_config();
    if ((dwt_getxtaltrim() == DEFAULT_XTAL_TRIM) || (rf_tuning->xtalTrim & ~XTAL_TRIM_BIT_MASK))
    {
        dwt_setxtaltrim(rf_tuning->xtalTrim & XTAL_TRIM_BIT_MASK);
    }

    // check if SIP device
    if (hal_uwb.is_sip())
    {
        struct uwbs_sip_config_s sip_cfg;
        dwt_config_t *dwt_config = get_dwt_config();
        sip_cfg.tx_ant = rf_tuning->tx_ant;
        sip_cfg.rxa_ant = rf_tuning->rxa_ant;
        sip_cfg.rxb_ant = rf_tuning->rxb_ant;
        sip_cfg.lna1 = rf_tuning->lna1;
        sip_cfg.lna2 = rf_tuning->lna2;
        sip_cfg.pa = rf_tuning->pa;
        sip_cfg.dual_rx = true;
        sip_cfg.channel = dwt_config->chan;
        hal_uwb.sip_configure(&sip_cfg);
    }

    /* set antenna delays */
    dwt_setrxantennadelay(rf_tuning->antRx_a);
    dwt_settxantennadelay(rf_tuning->antTx_a);

    dwt_setrxaftertxdelay(0); /**< no any delays set by default : part of config of receiver on Tx sending */
    dwt_setrxtimeout(0);      /**< no any delays set by default : part of config of receiver on Tx sending */
}
