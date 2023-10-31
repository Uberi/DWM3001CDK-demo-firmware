/**
 * @file      app.h
 *
 * @brief     Header file for app
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
#ifndef APP_H
#define APP_H
#include <stdbool.h>
#include <stdint.h>
#include "deca_error.h"

/* System mode of operation. used to
 *
 * 1. indicate in which mode of operation system is running
 * 2. configure the access rights to command handler in control mode
 * */
#define APP_SAVEABLE    0x10
#define APP_BLOCK_FLUSH 0x20
#define APP_LAST_SUB_CMD 0x40
typedef enum {
    mANY = 0,                   /**< Used only for Commands: indicates the command can be executed in any modes below */
    mIDLE = 1, 
    mAPP = 2,                 /**< IDLE mode */
    mAPPMASK   = 0x000F,
    mMASK      = 0xFFFF
}mode_e;

 typedef enum {
    NO_DATA = 0,
    DATA_READY,
    COMMAND_READY,
    DATA_SEND,
    DATA_FLUSH,
    DATA_STOP,
    DATA_SAVE,
    DATA_ERROR
 }usb_data_e;

struct command_s;

struct app_definition_s
{
    char *app_name;
    mode_e app_mode;
    void (*helper)(void const *argument);
    void (*terminate)(void);
    usb_data_e (*on_rx)(uint8_t *pBuf , uint16_t len, uint16_t *read_offset,uint16_t cyclic_size);
    void (*command_parser)(usb_data_e res, char * text);
    const struct command_s *sub_command;
};
typedef struct app_definition_s app_definition_t;
extern app_definition_t *known_apps;

error_e AppGetLastError(void);
void AppSetLastError(error_e error);
const app_definition_t *AppGet(void);
void AppSet(const app_definition_t *app);
const app_definition_t* AppGetDefaultEvent(void);
error_e AppSetDefaultEvent(const app_definition_t* app);
void AppConfigInit(void);
void app_apptimer_stop(void);

#endif