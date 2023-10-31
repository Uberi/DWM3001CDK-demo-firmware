/**
 * @file      fcfm_fn.c
 *
 * @brief
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

#include "EventManager.h"
#include "cmd.h"
#include "cmd_fn.h"
#include "tcfm.h"

const char COMMENT_TCFM[] = {"Test Continuous Frame mode is to transmit packets for test purposes.\r\nUsage: \"TCFM <NUM> <PAUSE> <LEN>\" <NUM>>: number of packets to transmit. <PAUSE>: pause in between packets in ms. <LEN>: length of the transmit payload, bytes\r\n"};
extern app_definition_t helpers_tcfm_node[];
/**
 * @brief   defaultTask will start TCFM user application
 * app.mode will be mTcfm
 * */
REG_FN(f_tcfm)
{
    char cmd[12];
    int n, nframes = 0, period = 0, nbytes = 0;

    n = sscanf(text, "%9s %d %d %d", cmd, &nframes, &period, &nbytes);

    switch (n)
    {
    case 1:
        // This is done to ensure a continuous frame mode for an infinite time
        nframes = 0;
        period = 1;  // 1 ms
        nbytes = 20; // 20 bytes packet
        break;
    case 2:
        period = 1;  // 1 ms
        nbytes = 20; // 20 bytes packet
        break;
    case 3:
        nbytes = 20; // 20 bytes packet
        break;
    default:
        break;
    }


    tcfm_info.period_ms = period;
    tcfm_info.bytes = nbytes;
    tcfm_info.nframes = nframes;

    app_definition_t *app_ptr = &helpers_tcfm_node[0];
    EventManagerRegisterApp((const app_definition_t **)&app_ptr);

    return (CMD_FN_RET_OK);
}


const struct command_s known_app_tcfm [] __attribute__((section(".known_commands_app")))= {
    {"TCFM",    mCmdGrp2 | mIDLE,  f_tcfm,               COMMENT_TCFM},
};
