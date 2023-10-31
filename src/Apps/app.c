/**
 * @file      app.c
 *
 * @brief     Application configuration
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
 */
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "app.h"
#include "HAL_error.h"
#include "appConfig.h"
#include "circular_buffers.h"
#include "cmd.h"
#include "crc16.h"


static error_e last_error = 0;

#ifdef CLI_BUILD
const app_definition_t idle_app[] = 
{
    {"STOP", mIDLE, NULL,  NULL, waitForCommand, command_parser}
};
#define DEFAULT_APP idle_app
#else
const app_definition_t idle_app[] = 
{
    {"STOP", mIDLE, NULL,  NULL, NULL, NULL}
};
extern const app_definition_t helpers_uci_node[];
#define DEFAULT_APP helpers_uci_node
#endif

static const app_definition_t *default_app __attribute__((section(".rconfig"))) = DEFAULT_APP;

static const app_definition_t *app = idle_app;

static void restore_default_app(void) 
{
    default_app  = DEFAULT_APP;
}

__attribute__((section(".config_entry"))) const void (*p_restore_default_app)(void) = (const void *)&restore_default_app;


error_e AppGetLastError(void)
{
    return last_error;
}

void AppSetLastError(error_e error)
{
    last_error = error;
}

const app_definition_t *AppGet(void)
{
    return app;
}

void AppSet(const app_definition_t *_app)
{
    if(_app == NULL)
    {
        app = (app_definition_t *)&DEFAULT_APP;
    }
    else
    {
        app = _app;
    }
}

const app_definition_t *AppGetDefaultEvent(void)
{
    return default_app;
}

error_e AppSetDefaultEvent(const app_definition_t *_app)
{
    default_app = _app;
    return save_bssConfig();
}

void AppConfigInit(void)
{
    init_crc16();
    load_bssConfig();                 /**< load the RAM Configuration parameters from NVM block */
}