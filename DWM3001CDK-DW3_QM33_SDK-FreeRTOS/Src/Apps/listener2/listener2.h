/**
 * @file      listener2.h
 *
 * @brief     Decawave
 *                bare implementation layer
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

#ifndef __LISTENER__H__
#define __LISTENER__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "uwb_frames.h"
#include "appConfig.h"
#include "HAL_uwb.h"

#define LISTENER_DATA 2

//-----------------------------------------------------------------------------
/*
 * Rx Events circular buffer : used to transfer RxPckt from ISR to APP
 * 0x02, 0x04, 0x08, 0x10, etc.
 * As per design, the amount of RxPckt in the buffer at any given time shall not be more than 1.
 * */
#define EVENT_BUF_L_SIZE      (0x10)


//-----------------------------------------------------------------------------


/* RxPckt is the structure is for the current reception */
struct rx_listener_pckt_s
{
    int16_t rxDataLen;

    union
    {
        std_msg_t         stdMsg;
        twr_msg_t         twrMsg;
        blink_msg_t       blinkMsg;
        rng_cfg_msg_t     rngCfgMsg;
        poll_msg_t        pollMsg;
        resp_pdoa_msg_t   respMsg;
        final_msg_accel_t finalMsg;
        uint8_t           data[STANDARD_FRAME_SIZE];
    } msg;

    uint8_t timeStamp[TS_40B_SIZE]; /* Full TimeStamp */

    /* Below is Decawave's diagnostics information */
    uint32_t status;
    int rsl100;
    int fsl100;
    int16_t clock_offset;
};

typedef struct rx_listener_pckt_s rx_listener_pckt_t;


/* This structure holds Listener's application parameters */
struct listener_info_s
{
    /* circular Buffer of received Rx packets :
     * uses in transferring of the data from ISR to APP level.
     * */
    struct
    {
        rx_listener_pckt_t buf[EVENT_BUF_L_SIZE];
        uint16_t           head;
        uint16_t           tail;
    } rxPcktBuf;

    dwt_deviceentcnts_t event_counts;
    uint32_t event_counts_sts_bad;    // counts the number of STS detections with bad STS quality
    uint32_t event_counts_sts_good;   // counts the number of STS detections with good STS quality
    uint32_t event_counts_sfd_detect; // counts the number of SFD detections (RXFR has to be set also)
};

typedef struct listener_info_s listener_info_t;

//-----------------------------------------------------------------------------
// HW-specific function implementation
//
void listener2_rssi_cal(int *rsl100, int *fsl100);
void listener2_readrxtimestamp(uint8_t *timestamp);
int listener2_readstsquality(int16_t *rxStsQualityIndex);
void listener2_configure_uwb(dwt_cb_t cbRxOk, dwt_cb_t cbRxTo, dwt_cb_t cbRxErr);
void listener2_deinit(void);


//-----------------------------------------------------------------------------
// exported functions prototypes
//
extern listener_info_t *getListenerInfoPtr(void);

//-----------------------------------------------------------------------------
// exported functions prototypes
//

/* responder (Listener) */

error_e listener_process_init(void);
void listener_process_start(void);
void listener_process_terminate(void);
void listener_set_mode(int mode);
int listener_get_mode(void);

//-----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif /* __LISTENER__H__ */
