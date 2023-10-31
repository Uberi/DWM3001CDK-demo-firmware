/**
 * @file      fira_fn.c
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

#include "cmd_fn.h"
#include "cmd.h"
#include "app.h"
#include "common_fira.h"
#include "usb_uart_tx.h"
#include "EventManager.h"
#include "reporter.h"
#include "rf_tuning_config.h"

#define INITF_OFFSET 0
#define RESPF_OFFSET 1

static const char COMMENT_FIRA_OPT[] = { "FiRa Options -----" };
static const char INITF_CMD_COMMENT[] = {
    "INITF [RFRAME BPRF set] [Slot duration rstu] [Block duration ms] [Round duration slots] [RR usage] [Session id] [vupper64 xx:xx:xx:xx:xx:xx:xx:xx] [Multi node mode] [Round hopping] [Initiator Addr] [Responder 1 Addr] ... [Responder n Addr]"};
static const char RESPF_CMD_COMMENT[] = {
    "RESPF [RFRAME BPRF set] [Slot duration rstu] [Block duration ms] [Round duration slots] [RR usage] [Session id] [vupper64 xx:xx:xx:xx:xx:xx:xx:xx] [Multi node mode] [Round hopping] [Initiator Addr] [Responder Addr]"};
static const char COMMENT_AVERAGE[] = {
    "Phase Difference Average. \r\nUsage: To see averaging value \"PAVRG\". To set the averaging value \"PAVRG <DEC>\""};

extern const app_definition_t helpers_app_fira[];

/* Fira Node and Tag */
REG_FN(f_initiator_f)
{
    const char *ret = CMD_FN_RET_OK;

    scan_fira_params(text, true);
    show_fira_params();

    const app_definition_t *app_ptr = &helpers_app_fira[INITF_OFFSET];
    EventManagerRegisterApp(&app_ptr);

    return (ret);
}

REG_FN(f_responder_f)
{
    const char *ret = CMD_FN_RET_OK;

    scan_fira_params(text, false);
    show_fira_params();

    const app_definition_t *app_ptr = &helpers_app_fira[RESPF_OFFSET];
    EventManagerRegisterApp(&app_ptr);

    return (ret);
}

REG_FN(f_pdoa_average)
{
    const char *ret = CMD_FN_RET_OK;

    char *str = CMD_MALLOC(MAX_STR_SIZE);

    int n, dummy;

    if (str)
    {
        rf_tuning_t *rf_tuning = get_rf_tuning_config();
        n = sscanf(text, "%9s %d", str, &dummy); // to count the number of arguments

        if (n == 2)
        {
            rf_tuning->paverage = (int16_t)val; // val is the input
        }

        int hlen;

        hlen = sprintf(str, "JS%04X", 0x5A5A);
        sprintf(&str[strlen(str)], "{\"AVERAGE\":%d}", rf_tuning->paverage);

        sprintf(&str[2], "%04X", strlen(str) - hlen);
        str[hlen] = '{';
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);

        ret = CMD_FN_RET_OK;
    }


    return (ret);
}


const struct command_s known_app_fira[] __attribute__((
    section(".known_commands_app"))) = {
    {"RESPF", mIDLE | mCmdGrp2, f_responder_f, RESPF_CMD_COMMENT},
    {"INITF", mIDLE | mCmdGrp2, f_initiator_f, INITF_CMD_COMMENT},
};

const struct command_s known_commands_fira[] __attribute__((
	section(".known_app_subcommands"))) = {
    { NULL, mCmdGrp0 | mIDLE, NULL, COMMENT_FIRA_OPT },
    { "PAVRG",mCmdGrp1 | mIDLE, f_pdoa_average,   COMMENT_AVERAGE},
};
