/**
 * @file      uci_backend_llp2mp.h
 *
 * @brief     headers for uci_backend_llp2mp.c
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

#ifndef UCI_BACKEND_LLP2MP_H
#define UCI_BACKEND_LLP2MP_H

#include "uci/uci.h"
#include "uwbmac_buf.h"
#include "uwbmac.h"
#include "lldc_helper.h"
#include "lldd_helper.h"

typedef enum
{
    lld_undefined = -1,
    lld_device = 0,
    lld_coordinator = 1
} lld_role_t;

enum uci_message_llp2mp_oid
{
    LLP2MP_RESET_CMD = 0b000000,
    LLP2MP_GET_INFO_CMD_RSP = 0b000010,
    LLP2MP_INIT_CMD_RSP = 0b001000,
    LLP2MP_DEINIT_CMD_RSP = 0b001001,
    LLP2MP_SEND_DATA_STD_CMD_RSP = 0b001010,
    LLP2MP_RECEIVE_DATA_STD_NTF = 0b001011,
    LLP2MP_NETWORK_NTF = 0b001110,
    LLP2MP_GET_STATS = 0b001111,
    LLP2MP_RESET_STATS = 0b010000,
};

int uci_llp2mp_platform_init(lld_role_t role, struct uwbmac_context *fira_uwbmac_ctx);
int uci_llp2mp_platform_deinit(void);
int uci_llp2mp_platform_data_tx(uint8_t *buf, uint8_t len);
int uci_llp2mp_platform_get_stats_coord(struct lldc_coord_stats *stats);
int uci_llp2mp_platform_get_stats_dev(struct lldd_stats *stats);
void uci_llp2mp_platform_reset_stats(void);
void uci_llp2mp_platform_get_rssi(float *rssi_a, float *rssi_b);
void uci_backend_llp2mp_send_notification(struct uwbmac_buf *buf, uint8_t oid);
void uci_backend_llp2mp_register(struct uci *uci, struct uwbmac_context *uwbmac_ctx);
void uci_backend_llp2mp_unregister(void);
#endif
