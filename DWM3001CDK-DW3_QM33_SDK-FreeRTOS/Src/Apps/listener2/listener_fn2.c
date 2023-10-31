/**
 * @file      listener_fn2.c
 *
 * @brief     Collection of executables functions from defined known_commands[]
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

#include <stdio.h>
#include <string.h>

/* sscanf not included in stdio.h... */
int sscanf(const char *str, const char *format, ...);

#include "cmd_fn.h"
#include "cmd.h"
#include "EventManager.h"
#include "listener2.h"
#include "reporter.h"

const char COMMENT_LISTENER_OPT[] = {"LISTENER Options -----"};
const char COMMENT_LSTAT[] = {"Displays the statistics inside the Listener application."};
const char COMMENT_LISTENER[] = {"Listen for the UWB packets using the UWB configuration.\r\nUsage: \"LISTENER2 <PARM>\" <PARM>: if present or 0, this is priority of speed. In this mode Listener will output maximum six first bytes from the input string. With <PARM> set to 1, the priority is on data and listener will output maximum 127 bytes of a payload."};

extern const app_definition_t helpers_app_listener[];

/**
 * @brief   defaultTask will start listener user application
 *
 * */
REG_FN(f_listen2)
{
    // Set mode to 2 for DW3_QM33_SDK
    listener_set_mode(2);

    app_definition_t *app_ptr = (app_definition_t *)&helpers_app_listener[0];
    EventManagerRegisterApp((void *)&app_ptr);

    return (CMD_FN_RET_OK);
}

REG_FN(f_lstat)
{
    char *str = CMD_MALLOC(MAX_STR_SIZE);

    if (str)
    {
        CMD_ENTER_CRITICAL();
        int hlen;
        listener_info_t *info = getListenerInfoPtr();
        /** Listener RX Event Counts object */
        hlen = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object
        sprintf(&str[strlen(str)], "{\"RX Events\":{\r\n");
        sprintf(&str[strlen(str)], "\"CRCG\":%d,\r\n", (int)info->event_counts.CRCG);
        sprintf(&str[strlen(str)], "\"CRCB\":%d,\r\n", (int)info->event_counts.CRCB);
        sprintf(&str[strlen(str)], "\"ARFE\":%d,\r\n", (int)info->event_counts.ARFE);
        sprintf(&str[strlen(str)], "\"PHE\":%d,\r\n", (int)info->event_counts.PHE);
        sprintf(&str[strlen(str)], "\"RSL\":%d,\r\n", (int)info->event_counts.RSL);
        sprintf(&str[strlen(str)], "\"SFDTO\":%d,\r\n", (int)info->event_counts.SFDTO);
        sprintf(&str[strlen(str)], "\"PTO\":%d,\r\n", (int)info->event_counts.PTO);
        sprintf(&str[strlen(str)], "\"FTO\":%d,\r\n", (int)info->event_counts.RTO);
        sprintf(&str[strlen(str)], "\"STSE\":%d,\r\n", (int)info->event_counts_sts_bad);
        sprintf(&str[strlen(str)], "\"STSG\":%d,\r\n", (int)info->event_counts_sts_good);
        sprintf(&str[strlen(str)], "\"SFDD\":%d}}", (int)info->event_counts_sfd_detect);
        sprintf(&str[2], "%04X", strlen(str) - hlen); // add formatted 4X of length, this will erase first '{'
        str[hlen] = '{';                              // restore the start bracket
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);

        CMD_EXIT_CRITICAL();
    }
    return (CMD_FN_RET_OK);
}

const struct command_s known_app_listener[] __attribute__((section(".known_commands_app"))) = {
    {"LISTENER2", mCmdGrp2 | mIDLE,  f_listen2,             COMMENT_LISTENER},
};

const struct command_s known_commands_listener[] __attribute__((section(".known_app_subcommands"))) = {
    {NULL,      mCmdGrp0 | mAPP,                    NULL,    COMMENT_LISTENER_OPT},
    {"LSTAT",   mCmdGrp1 | mAPP | APP_LAST_SUB_CMD, f_lstat, COMMENT_LSTAT  },
};