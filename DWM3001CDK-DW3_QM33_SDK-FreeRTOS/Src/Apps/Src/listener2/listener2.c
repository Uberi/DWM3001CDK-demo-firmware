/**
 * @file      listener2.c
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
#include <stdlib.h>

#include "cmsis_os.h"
#include "critical_section.h"

#include "listener2.h"
#include "deca_device_api.h"
#include "deca_dbg.h"
#include "circular_buffers.h"
#include "HAL_error.h"
#include "HAL_rtc.h"
#include "HAL_uwb.h"
#include "minmax.h"
#include "task_listener2.h"
#include "driver_app_config.h"
#include "rf_tuning_config.h"

// ----------------------------------------------------------------------------
// implementation-specific: critical section protection
#ifndef TWR_ENTER_CRITICAL
#define TWR_ENTER_CRITICAL enter_critical_section
#endif

#ifndef TWR_EXIT_CRITICAL
#define TWR_EXIT_CRITICAL leave_critical_section
#endif

#define NODE_MALLOC malloc
#define NODE_FREE   free


//-----------------------------------------------------------------------------
// The psListenerInfo structure holds all Listener's process parameters
static listener_info_t *psListenerInfo = NULL;


/*
 * @brief     get pointer to the twrInfo structure
 * */
listener_info_t *getListenerInfoPtr(void)
{
    return (psListenerInfo);
}

static int listener_mode = 0;
void listener_set_mode(int mode)
{
    listener_mode = mode;
}

int listener_get_mode(void)
{
    return listener_mode;
}

/*
 * @brief     ISR level (need to be protected if called from APP level)
 *             low-level configuration for DW3000
 *
 *             if called from app, shall be performed with DW IRQ off &
 *             TWR_ENTER_CRITICAL(); / TWR_EXIT_CRITICAL();
 *
 *             The SPI for selected chip shall already be chosen
 *
 *
 *
 * @note
 *     This function uses common UWB APIs to DW3000 and QM3572X, so it is kept here.
 * */
static void
rxtx_listener_configure
(
    dwt_config_t    *pdwCfg,
    uint16_t        frameFilter,
    uint16_t        txAntDelay,
    uint16_t        rxAntDelay
)
{

    if (dwt_configure(pdwCfg)) /**< Configure the Physical Channel parameters (PLEN, PRF, etc) */
    {
        error_handler(1, _ERR_INIT);
    }
    dwt_setrxaftertxdelay(0); /**< no any delays set by default : part of config of receiver on Tx sending */
    dwt_setrxtimeout(0);      /**< no any delays set by default : part of config of receiver on Tx sending */
    dwt_configureframefilter(DWT_FF_DISABLE, 0);
}


/* @brief   ISR level
 *          TWR application Rx callback
 *          to be called from dwt_isr() as an Rx call-back
 * */
void rx_listener_cb(const dwt_cb_data_t *rxd)
{
    listener_info_t *pListenerInfo = getListenerInfoPtr();

    if (!pListenerInfo)
    {
        return;
    }

    int16_t stsQual;

    const int size = sizeof(pListenerInfo->rxPcktBuf.buf) / sizeof(pListenerInfo->rxPcktBuf.buf[0]);

    int head = pListenerInfo->rxPcktBuf.head;
    int tail = pListenerInfo->rxPcktBuf.tail;

    if (CIRC_SPACE(head, tail, size) > 0)
    {
        rx_listener_pckt_t *p = &pListenerInfo->rxPcktBuf.buf[head];

        listener2_readrxtimestamp(p->timeStamp); // Raw Rx TimeStamp (STS or IPATOV based on STS config)

        p->clock_offset = dwt_readclockoffset(); // Reading Clock offset for any Rx packets

        p->status = rxd->status;

        if (listener2_readstsquality(&stsQual) < 0) // if < 0 then this is "bad" STS
        {
            pListenerInfo->event_counts_sts_bad++;
        }
        else
        {
            pListenerInfo->event_counts_sts_good++;
        }

        pListenerInfo->event_counts_sfd_detect++;

        // check if this is an SP3 packet
        if (rxd->rx_flags & DWT_CB_DATA_RX_FLAG_ND)
        {
            p->rxDataLen = 0;
        }
        else
        {
            p->rxDataLen = MIN(rxd->datalength, sizeof(p->msg));

            dwt_readrxdata((uint8_t *)&p->msg, p->rxDataLen, 0); // Raw message

            // p->msg.data[p->rxDataLen] = stsQual>>8;
            // p->msg.data[p->rxDataLen+1] = stsQual;
            // p->rxDataLen+=2;
        }

        listener2_rssi_cal(&p->rsl100, &p->fsl100);

        if (listener_task_started()) // RTOS : listenerTask can be not started yet
        {
            head = (head + 1) & (size - 1);
            pListenerInfo->rxPcktBuf.head = head; // ISR level : do not need to protect
        }
    }

    listener_task_notify();

    sts_config_t *sts_config = get_sts_config();
    if (sts_config->stsInteropMode) // value 0 = dynamic STS, 1 = fixed STS)
    {
        // re-load the initial cp_iv value to keep STS the same for each frame
        dwt_configurestsloadiv();
    }

    dwt_readeventcounters(&pListenerInfo->event_counts); // take a snapshot of event counters

    dwt_rxenable(DWT_START_RX_IMMEDIATE); // re-enable receiver again - no timeout


    /* ready to serve next raw reception */
}


void listener_timeout_cb(const dwt_cb_data_t *rxd)
{
    listener_info_t *pListenerInfo = getListenerInfoPtr();
    sts_config_t *sts_config = get_sts_config();
    if (sts_config->stsInteropMode) // value 0 = dynamic STS, 1 = fixed STS)
    {
        // re-load the initial cp_iv value to keep STS the same for each frame
        dwt_configurestsloadiv();
    }

    dwt_readeventcounters(&pListenerInfo->event_counts);

    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

void listener_error_cb(const dwt_cb_data_t *rxd)
{
    listener_timeout_cb(rxd);
}


//-----------------------------------------------------------------------------

/* @brief     app level
 *     RTOS-independent application level function.
 *     initializing of a TWR Node functionality.
 *
 * */
error_e listener_process_init()
{
    if (!psListenerInfo)
    {
        psListenerInfo = NODE_MALLOC(sizeof(listener_info_t));
    }

    listener_info_t *pListenerInfo = getListenerInfoPtr();

    if (!pListenerInfo)
    {
        return (_ERR_Cannot_Alloc_NodeMemory);
    }

    /* switch off receiver's rxTimeOut, RxAfterTxDelay, delayedRxTime,
     * autoRxEnable, dblBufferMode and autoACK,
     * clear all initial counters, etc.
     * */
    memset(pListenerInfo, 0, sizeof(listener_info_t));

    memset(&pListenerInfo->event_counts, 0, sizeof(pListenerInfo->event_counts));

    pListenerInfo->event_counts_sts_bad = 0;
    pListenerInfo->event_counts_sts_good = 0;
    pListenerInfo->event_counts_sfd_detect = 0;

    /* Configure non-zero initial variables.1 : from app parameters */

    /* The Listener has its configuration in the app->pConfig, see DEFAULT_CONFIG.
     *
     *
     * */
    dwt_config_t *dwt_config = get_dwt_config();

    /* dwt_xx calls in app level Must be in protected mode (DW3000 IRQ disabled) */
    hal_uwb.disableIRQ();

    TWR_ENTER_CRITICAL();

    if (dwt_initialise(0) != DWT_SUCCESS) /**< set callbacks to NULL inside dwt_initialise*/
    {
        return (_ERR_INIT);
    }

    if (hal_uwb.uwbs != NULL)
    {
        hal_uwb.uwbs->spi->fast_rate(hal_uwb.uwbs->spi->handler);
    }

    if (hal_uwb.is_aoa() == AOA_ENABLED)
    {
        diag_printf("Found AOA DW3000 chip. PDoA is available.\r\n");
    }
    else if (hal_uwb.is_aoa() == AOA_DISABLED)
    {
        dwt_config->pdoaMode = DWT_PDOA_M0;
        diag_printf("Found non-AOA DW3000 chip. PDoA is not available.\r\n");
    }
    else
    {
        diag_printf("Found unknown DW3000 chip 0x%04X. Stop.\r\n", (unsigned int)dwt_readdevid());
        return DWT_ERROR;
    }


    /* Configure DW IC's UWB mode, sets power and antenna delays for TWR mode
     * Configure SPI to fast rate */
    rf_tuning_t *rf_tuning = get_rf_tuning_config();
    rxtx_listener_configure(dwt_config,
                            DWT_FF_DISABLE, /* No frame filtering for Listener */
                            rf_tuning->antTx_a,
                            rf_tuning->antRx_a);
    listener2_configure_uwb(rx_listener_cb, listener_timeout_cb, listener_error_cb);

    /* End configuration of DW IC */

    {
        /* configure the RTC Wakeup timer with a high priority;
         * this timer is saving global Super Frame Timestamp,
         * so we want this timestamp as stable as we can.
         *
         * */
        Rtc.disableIRQ();
        Rtc.setPriorityIRQ();
    }

    TWR_EXIT_CRITICAL();

    return (_NO_ERR);
}

/*
 * @brief
 *     Enable DW3000 IRQ to start
 * */
void listener_process_start(void)
{
    hal_uwb.enableIRQ();
    Rtc.enableIRQ(); // start the RTC timer
    diag_printf("Listener Top Application: Started\r\n");
}


/* @brief     app level
 *     RTOS-independent application level function.
 *     deinitialize the pListenerInfo structure.
 *    This must be executed in protected mode.
 *
 * */
void listener_process_terminate(void)
{
    listener2_deinit();
    // stop the RTC timer
    /* configure the RTC Wakeup timer with a high priority;
     * this timer is saving global Super Frame Timestamp,
     * so we want this timestamp as stable as we can.
     *
     * */
    Rtc.disableIRQ();
    Rtc.setPriorityIRQ();

    if (psListenerInfo)
    {
        NODE_FREE(psListenerInfo);
        psListenerInfo = NULL;
    }
}

//-----------------------------------------------------------------------------
