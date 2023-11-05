/**
 * @file      cmd_rf_tuning.c
 *
 * @brief     FiRa Options commands handlers
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
#include "cmd_fn.h"
#include "reporter.h"
#include "rf_tuning_config.h"
#include "deca_dbg.h"

const char COMMENT_PDOAOFF         []={"Phase Difference offset for this Node\r\nUsage: To see Phase Difference offset value \"PDOAOFF\". To set the Phase Difference offset value \"PDOAOFF <DEC>\""};

const char COMMENT_ANTTXA          []={"Antenna TX delay \r\nUsage: \"ANTTXA <DEC>\""};
const char COMMENT_ANTRXA          []={"Antenna RX delay \r\nUsage: \"ANTRXA <DEC>\""};
const char COMMENT_ANTRXB          []={"For future use"};

const char COMMENT_XTALTRIM        []={"Xtal trimming value.\r\nUsage: To see Crystal Trim value \"XTALTRIM\". To set the Crystal trim value [0..7F] \"XTALTRIM 0x<HEX>\""};

// TODO: the current MAC only uses the TX antenna delay on QM33
REG_FN(f_ant_tx_a)
{
    uint8_t n = 0;
    unsigned int dly = 0;
    char cmd[12];
    rf_tuning_t *rf_tuning = get_rf_tuning_config();

    n = sscanf(text, "%9s %u", cmd, &dly);

    if (n != 2)
    {
        diag_printf("ANT_TXA: %d \r\n", rf_tuning->antTx_a);
    }
    else
    {
        rf_tuning->antTx_a = (uint16_t)(dly);
    }
    return (CMD_FN_RET_OK);
}

REG_FN(f_ant_rx_a)
{
    uint8_t n = 0;
    unsigned int dly = 0;
    char cmd[12];
    rf_tuning_t *rf_tuning = get_rf_tuning_config();

    n = sscanf(text, "%9s %u", cmd, &dly);

    if (n != 2)
    {
        diag_printf("ANT_RXA: %d \r\n", rf_tuning->antRx_a);
    }
    else
    {
        rf_tuning->antRx_a = (uint16_t)(dly);
    }
    return (CMD_FN_RET_OK);
}

// TODO: the current MAC does not use the RXB antenna delay.
#ifndef UWBSTACK
REG_FN(f_ant_rx_b)
{
    uint8_t n = 0;
    unsigned int dly = 0;
    char cmd[12];
    rf_tuning_t *rf_tuning = get_rf_tuning_config();

    n = sscanf(text, "%9s %u", cmd, &dly);

    if (n != 2)
    {
        diag_printf("ANT_RXB: %d \r\n", rf_tuning->antRx_b);
    }
    else
    {
        rf_tuning->antRx_b = (uint16_t)(dly);
    }
    return (CMD_FN_RET_OK);
}
#endif

REG_FN(f_pdoa_offset)
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
            rf_tuning->pdoaOffset_deg = (int16_t)val; // val is the input
        }

        int hlen;

        hlen = sprintf(str, "JS%04X", 0x5A5A);
        sprintf(&str[strlen(str)], "{\"PDOAOFF_deg\":%d}", rf_tuning->pdoaOffset_deg);

        sprintf(&str[2], "%04X", strlen(str) - hlen);
        str[hlen] = '{';
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);

        ret = CMD_FN_RET_OK;
    }

    return (ret);
}

/**
 * @brief set or show current XTAL TRIM in JSON format
 * @param no param - show current Xtal Trim code
 *        correct scanned string - set the Trim code
 *        incorrect scanned string - do not set XTAL TRIM
 *
 * */
REG_FN(f_xtal_trim)
{
    const char *ret = CMD_FN_RET_OK;

    char *str = CMD_MALLOC(MAX_STR_SIZE);

    int n, xtalTrim;

    if (str)
    {
        rf_tuning_t *rf_tuning = get_rf_tuning_config();
        n = sscanf(text, "%9s 0X%02x", str, &xtalTrim);

        CMD_ENTER_CRITICAL();

        if (n == 2)
        {
            dwt_setxtaltrim((uint8_t)xtalTrim & 0x7F);
        }
        else
        {
            dwt_setxtaltrim(rf_tuning->xtalTrim & XTAL_TRIM_BIT_MASK);
            xtalTrim = dwt_getxtaltrim() | (rf_tuning->xtalTrim & ~XTAL_TRIM_BIT_MASK);
        }

        rf_tuning->xtalTrim = xtalTrim; // it can have the 0x80 bit set to be able overwrite OTP values during APP starts.

        CMD_EXIT_CRITICAL();

        /* Display the XTAL object */
        int hlen;

        hlen = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object
        sprintf(&str[strlen(str)], "{\"XTAL\":{\r\n");
        sprintf(&str[strlen(str)], "\"TEMP TRIM\":\"0x%02x\"}}", xtalTrim);

        sprintf(&str[2], "%04X", strlen(str) - hlen); // add formatted 4X of length, this will erase first '{'
        str[hlen] = '{';                              // restore the start bracket
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);
    }

    return (ret);
}

const struct command_s known_commands_idle_rf[] __attribute__((section(".known_commands_ilde"))) = {
/* TODO: the current MAC does not use the RXB antenna delay. */
    {"ANTTXA",  mCmdGrp1 | mIDLE,  f_ant_tx_a,              COMMENT_ANTTXA},
    {"ANTRXA",  mCmdGrp1 | mIDLE,  f_ant_rx_a,              COMMENT_ANTRXA},
#ifndef UWBSTACK
    {"ANTRXB",  mCmdGrp1 | mIDLE,  f_ant_rx_b,              COMMENT_ANTRXB},
#endif
    {"XTALTRIM",mCmdGrp1 | mIDLE,  f_xtal_trim,             COMMENT_XTALTRIM},
    {"PDOAOFF", mCmdGrp1 | mIDLE,  f_pdoa_offset,           COMMENT_PDOAOFF},
};
