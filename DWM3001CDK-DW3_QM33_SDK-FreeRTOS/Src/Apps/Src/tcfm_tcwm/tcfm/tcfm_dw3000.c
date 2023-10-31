/**
 * @file     tcfm.c
 *
 * @brief    process to run Test Continuous Frame Mode
 *
 *           This test application will send a number of packets (e.g. 200) and then stop
 *           The payload and the inter-packet period can also be varied
 *           command: "TCFM N D P", where N is number of packets to TX, D is inter packet period (in ms), P is payload in bytes
 *
 *
 *    measure the power:
 *    Spectrum Analyser set:
 *    FREQ to be channel default e.g. 6489.6 MHz for channel 5, 7987.2 MHz for channel 9
 *    SPAN to 1GHz
 *    SWEEP TIME 1s
 *    RBW and VBW 1MHz
 *    measure channel power
 *    measure peak power
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

#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "appConfig.h"
#include "tcfm.h"
//#include "msg_time.h"
#include "uwb_frames.h"
#include "deca_device_api.h"
#include "EventManager.h"
#include "HAL_uwb.h"
#include "driver_app_config.h"
#include "rf_tuning_config.h"

#define DW_MS_PERIOD 249600 /* 1 ms in 4ns units to program into DX_TIME_ID */

tcfm_info_t tcfm_info;
static int period;

struct tcfm_app_s
{
    uint8_t payload[127];
    uint16_t msg_len;
    uint16_t msg_count;
    uint16_t nframes;
    uint8_t fixed_sts;
};

static struct tcfm_app_s *pTcfmMsg = NULL;


/* IMPLEMENTATION */

/* @brief   ISR level
 *          TCFM application TX callback
 *          to be called from dwt_isr() as an TX call-back
 * */
void tcfm_tx_cb(const dwt_cb_data_t *rxd)
{
    if (!pTcfmMsg)
    {
        return;
    }
    // Add condition to allow the continuous frame mode when nframes is not mentionned as parameter
    if (pTcfmMsg->msg_count >= pTcfmMsg->nframes && pTcfmMsg->nframes != 0)
    {
        // we have transmitted required number of messages - stop the application

        // FreeRTOS specific implementation of how-to set the Event from the ISR
        // bool xHigherPriorityTaskWoken;
        bool xResult;

        // xHigherPriorityTaskWoken = false;

        app_definition_t *app_ptr = (app_definition_t *)&idle_app[0];
        xResult = EventManagerRegisterAppFromISR((const app_definition_t **)&app_ptr);
        // Was the message posted successfully?
        if (xResult == true)
        {
            // If xHigherPriorityTaskWoken is now set to pdTRUE then a context
            // switch should be requested.  The macro used is port specific and
            // will be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() -
            // refer to the documentation page for the port being used.
            // This function is not known for zephyr OS.
            // portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
    else
    {
        pTcfmMsg->msg_count++;

        pTcfmMsg->payload[1] = pTcfmMsg->msg_count; // packet counter 16-bit
        pTcfmMsg->payload[2] = pTcfmMsg->msg_count >> 8;

        dwt_writetxdata(pTcfmMsg->msg_len, (uint8_t *)pTcfmMsg->payload, 0);

        dwt_writetxfctrl(pTcfmMsg->msg_len, 0, 0);

        // This is done to ensure a continious frame transmission
        // To resolve known bug in silicon
        dwt_setdelayedtrxtime(DW_MS_PERIOD * period);


        // the configured delay in between TX packets was set in the init
        if (pTcfmMsg->fixed_sts)
        {
            // re-load the initial cp_iv value to keep STS the same for each frame
            dwt_configurestsloadiv();
        }
        dwt_starttx(DWT_START_TX_DLY_TS);
    }
}

/*
 * @brief     init function initialises all run-time environment allocated by the process
 *             it will be executed once
 *
 * */
error_e tcfm_process_init(tcfm_info_t *info)
{
    error_e ret = _NO_ERR;

    pTcfmMsg = malloc(sizeof(struct tcfm_app_s));

    if (!pTcfmMsg)
    {
        return _ERR_Cannot_Alloc_Memory;
    }

    // define some test data for the tx buffer
    const uint8_t msg_data[] = "The quick brown fox jumps over the lazy dog";

    // configure device settings based on the settings stored in app.pConfig
    // it will not return if the init will fail
    tcXm_configure_test_mode();

    dwt_set_alternative_pulse_shape(1);

    dwt_setcallbacks(tcfm_tx_cb, NULL, NULL, NULL, NULL, NULL, NULL);

    dwt_setinterrupt(DWT_INT_TXFRS_BIT_MASK, 0, DWT_ENABLE_INT_ONLY);

    hal_uwb.irq_init(); /**< manually init EXTI DW3000 lines IRQs */

    sts_config_t *sts_config = get_sts_config();
    pTcfmMsg->fixed_sts = sts_config->stsInteropMode; // value 0 = dynamic STS, 1 = fixed STS

    // configure STS KEY/IV
    dwt_configurestskey(&sts_config->stsKey);
    dwt_configurestsiv(&sts_config->stsIv);
    // load the configured KEY/IV values
    dwt_configurestsloadiv();

    /* Setup Tx packet*/
    pTcfmMsg->msg_len = (uint16_t)info->bytes; // overall message length
    pTcfmMsg->msg_count = 1;
    pTcfmMsg->nframes = (uint16_t)info->nframes;

    memcpy(pTcfmMsg->payload, msg_data, info->bytes);


    /*
     * The dwt_initialize will read the default XTAL TRIM from the OTP or use the DEFAULT_XTAL_TRIM.
     * In this case we would apply the user-configured value.
     *
     * Bit 0x80 can be used to overwrite the OTP settings if any.
     * */
    rf_tuning_t *rf_tuning = get_rf_tuning_config();
    if ((dwt_getxtaltrim() == DEFAULT_XTAL_TRIM) || (rf_tuning->xtalTrim & ~XTAL_TRIM_BIT_MASK))
    {
        dwt_setxtaltrim(rf_tuning->xtalTrim & XTAL_TRIM_BIT_MASK);
    }


    if (info->bytes == 5) // Special interop case
    {
        pTcfmMsg->payload[0] = 0x10;
        pTcfmMsg->payload[1] = pTcfmMsg->msg_count; // packet counter 16-bit
        pTcfmMsg->payload[2] = pTcfmMsg->msg_count >> 8;
    }

    dwt_writetxdata(pTcfmMsg->msg_len, (uint8_t *)pTcfmMsg->payload, 0);

    dwt_writetxfctrl(pTcfmMsg->msg_len, 0, 0);

    // If the length of the packet is > period_ms, the packets will be sent back-to-back
    dwt_setdelayedtrxtime(DW_MS_PERIOD * info->period_ms);
    period = info->period_ms;

    return ret;
}

/*
 * @brief     run function implements continuous process functionality
 * */
void tcfm_process_run(void)
{
    /*do nothing*/
}


/*
 * @brief     stop function implements stop functionality if any
 *             which will be executed on reception of Stop command
 * */
void tcfm_process_terminate(void)
{
    hal_uwb.stop_all_uwb();

    if (pTcfmMsg)
    {
        free(pTcfmMsg);
        pTcfmMsg = NULL;
    }
}
