/**
 * @file      driver_app_config.h
 *
 * @brief     Driver config file for NVM initialization
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

#include <string.h>

#include "default_config.h"
#include "driver_app_config.h"

struct dwt_app_config_s
{
    dwt_config_t dwt_config; /**< Standard Decawave driver config */
    sts_config_t sts_config;
};

typedef struct dwt_app_config_s dwt_app_config_t;
static const dwt_app_config_t dwt_app_config_flash_default =
{
    .dwt_config.chan           = DEFAULT_CHANNEL,
    .dwt_config.txPreambLength = DEFAULT_TXPREAMBLENGTH,
    .dwt_config.rxPAC          = DEFAULT_RXPAC,
    .dwt_config.txCode         = DEFAULT_PCODE,
    .dwt_config.rxCode         = DEFAULT_PCODE,
    .dwt_config.sfdType        = DEFAULT_NSSFD,
    .dwt_config.dataRate       = DEFAULT_DATARATE,
    .dwt_config.phrMode        = DEFAULT_PHRMODE,
    .dwt_config.phrRate        = DEFAULT_PHRRATE,
    .dwt_config.sfdTO          = DEFAULT_SFDTO,
    .dwt_config.stsMode        = DEFAULT_STS_MODE,
    .dwt_config.stsLength      = DEFAULT_STS_LENGTH,
    .dwt_config.pdoaMode       = DEFAULT_PDOA_MODE,
    .sts_config.stsKey.key0    = 0x14EB220FUL,
    .sts_config.stsKey.key1    = 0xF86050A8UL,
    .sts_config.stsKey.key2    = 0xD1D336AAUL,
    .sts_config.stsKey.key3    = 0x14148674UL,
    .sts_config.stsIv.iv0      = 0x1F9A3DE4UL,
    .sts_config.stsIv.iv1      = 0xD37EC3CAUL,
    .sts_config.stsIv.iv2      = 0xC44FA8FBUL,
    .sts_config.stsIv.iv3      = 0x362EEB34UL,
    .sts_config.stsInteropMode = 1L,
};

static dwt_app_config_t dwt_app_config_ram __attribute__((section(".rconfig"))) = {0};

dwt_config_t *get_dwt_config(void)
{
    return &dwt_app_config_ram.dwt_config;
}

sts_config_t *get_sts_config(void)
{
    return &dwt_app_config_ram.sts_config;
}

static void restore_dwt_app_default_config(void)
{
    memcpy(&dwt_app_config_ram, &dwt_app_config_flash_default, sizeof(dwt_app_config_ram));
}

__attribute__((section(".config_entry"))) const void (*p_restore_driver_app_default_config)(void) = (const void *)&restore_dwt_app_default_config;
