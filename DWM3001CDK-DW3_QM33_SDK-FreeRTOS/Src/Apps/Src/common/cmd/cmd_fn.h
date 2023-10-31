/**
 * @file      cmd_fn.h
 *
 * @brief     Header file for macros, structures and protypes cmd_fn.c
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

#ifndef INC_CMD_FN_H_
#define INC_CMD_FN_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#include "app.h"
#include "cJSON.h"
#include "cmsis_os.h"
#include "critical_section.h"
#include "default_config.h"

//-----------------------------------------------------------------------------
/* module DEFINITIONS */
#define MAX_STR_SIZE 255

#define CMD_MALLOC           malloc
#define CMD_FREE             free
#define CMD_ENTER_CRITICAL() enter_critical_section()
#define CMD_EXIT_CRITICAL()  leave_critical_section()

typedef enum
{
    mCmdGrp0 = (0x1 << 16), /* This group of commands is a delimiter */
    mCmdGrp1 = (0x2 << 16), /* This group of commands has format #1 */
    mCmdGrp2 = (0x4 << 16), /* This group specifies the application */
    mCmdGrp3 = (0x8 << 16), /* below reserved for the future */
    mCmdGrp4 = (0x10 << 16),
    mCmdGrp5 = (0x20 << 16),
    mCmdGrp6 = (0x40 << 16),
    mCmdGrp7 = (0x80 << 16),
    mCmdGrp8 = (0x100 << 16),
    mCmdGrp9 = (0x200 << 16),
    mCmdGrp10 = (0x400 << 16),
    mCmdGrp11 = (0x800 << 16),
    mCmdGrp12 = (0x1000 << 16),
    mCmdGrp13 = (0x2000 << 16),
    mCmdGrp14 = (0x4000 << 16),
    mCmdGrp15 = (0x8000 << 16),
    mCmdGrpMASK = 0xFFFF0000L
} cmdGroup_e;


//-----------------------------------------------------------------------------
/* All cmd_fn functions have unified input: (char *text, param_block_t *pbss, int val) */
/* use REG_FN(x) macro */
#define REG_FN(x) const char *x(char *text, void *pbss, int val, cJSON *params)

/* command table structure definition */
struct command_s
{
    const char *name;    /**< Command name string */
    const uint32_t mode; /**< allowed execution operation mode */
    REG_FN((*fn));       /**< function() */
    const char *cmnt;
};

typedef struct command_s command_t;
extern command_t *known_commands;
extern const char CMD_FN_RET_OK[];
extern const char CMD_FN_RET_KO[];
extern const char COMMENT_VERSION[];

void command_stop_received(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_CMD_FN_H_ */
