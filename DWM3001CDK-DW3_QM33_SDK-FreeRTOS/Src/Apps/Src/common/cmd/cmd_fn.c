/**
 * @file      cmd_fn.c
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "reporter.h"
#include "cmd_fn.h"
#include "thisBoard.h"
#include "translate.h"
#include "rtls_version.h"
#include "appConfig.h"
#include "flushTask.h"
#include "deca_dbg.h"
#include "usb_uart_tx.h"
#include "cmd.h"
#include "EventManager.h"
#include "HAL_timer.h"
#include "HAL_error.h"
#include "HAL_uart.h"
#include "thread_fn.h"
#include "driver_app_config.h"
#include "debug_config.h"
#include "comm_config.h"
#include "rf_tuning_config.h"
#include "HAL_uwb.h"

#define CMD_COLUMN_WIDTH 10
#define CMD_COLUMN_MAX   4
//-----------------------------------------------------------------------------
const char CMD_FN_RET_OK[] = "ok\r\n";
const char CMD_FN_RET_KO[] = "KO\r\n";

/* Antenna pair structure, to correlate antenna type enum with its string name */
struct antenna_pair_s
{
    antenna_type_e antenna_type; /**< Antenna type */
    char *name;                  /**< String to parse/report the correlated type */
};
typedef struct antenna_pair_s antenna_pair_t;

/* Possible antenna type values (enum and string) */
const antenna_pair_t antenna_list[] =
{
    { ANT_TYPE_NONE      ,"NONE" },
    { ANT_TYPE_MAN5      ,"MAN5" },
    { ANT_TYPE_CPWING5   ,"CPWING5" },
    { ANT_TYPE_CPWING9   ,"CPWING9" },
    { ANT_TYPE_MONALISA5 ,"MONALISA5" },
    { ANT_TYPE_MONALISA9 ,"MONALISA9" },
    { ANT_TYPE_JOLIE5    ,"JOLIE5" },
    { ANT_TYPE_JOLIE9    ,"JOLIE9" },
    { ANT_TYPE_CUSTOM    ,"CUSTOM" },
    { 0                  ,NULL },
};

const char COMMENT_ANYTIME_OPTIONS []={"Anytime commands -----"};
const char COMMENT_APPSELECTION    []={"Application selection "};
const char COMMENT_SERVICE         []={"Service commands -----"};
const char COMMENT_IDLETIME_OPTIONS []={"IDLE time commands --"};

const struct command_s known_commands_anytime [] = {
   {NULL,      mCmdGrp0 | mANY,   NULL ,                   COMMENT_ANYTIME_OPTIONS},
};

const struct command_s known_commands_app_start [] = {
    {NULL,         mCmdGrp0 | mIDLE,  NULL ,                COMMENT_APPSELECTION},
};
static const struct command_s known_commands_idle [] = {
    {NULL,      mCmdGrp0 | mIDLE,   NULL ,                  COMMENT_IDLETIME_OPTIONS},
};

static const struct command_s known_commands_service [] = {
    /** 5. service commands */
    {NULL,      mCmdGrp0 | mIDLE,  NULL ,                   COMMENT_SERVICE},
};

#define NUMBER_OF_ANT_PORTS   (int)sizeof(antenna_t)

__attribute__((weak)) char *f_jstat(char *text, void *pbss, int val, cJSON *params)
{
    return NULL;
};

__attribute__((weak)) char *f_get_known_list(char *text, void *pbss, int val, cJSON *params)
{
    return NULL;
};

__attribute__((weak)) char *f_get_discovered_list(char *text, void *pbss, int val, cJSON *params)
{
    return NULL;
};

extern const app_definition_t idle_app[];
/***************************************************************************/
/*
 *
 *                          f_xx "command" FUNCTIONS
 *
 * REG_FN(f_node) macro will create a function
 *
 * const char *f_node(char *text, param_block_t *pbss, int val)
 *
 */

//-----------------------------------------------------------------------------
// Operation Mode change section

/** @defgroup Application_Selection
 * @brief    Commands to start the particular application
 * @{
 */

// communication to the user application
void command_stop_received(void)
{
    const app_definition_t *app_ptr = &idle_app[0];
    EventManagerRegisterApp(&app_ptr);
}

/**
 * @brief    defaultTask will stop all working threads
 * AppGet()->app_mode will be mIDLE
 * */
REG_FN(f_stop)
{
    FlushTask_reset();
    reporter_instance.print((char *)"\r\n", 2);
    const app_definition_t *app_ptr = &idle_app[0];
    EventManagerRegisterApp(&app_ptr);
    return (CMD_FN_RET_OK);
}

/**
 * @brief show current threads and stack depth
 *
 * */
REG_FN(f_thread)
{
    return thread_fn();
}

/**
 * @}
 */


/** @defgroup Application_Parameters
 * @brief Parameters change section : allowed only in AppGet()->app_mode = mIdle
 * @{
 */

REG_FN(f_diag)
{
    debug_config_t *debug_config = get_debug_config();
    debug_config->diagEn = (uint8_t)val;
    return (CMD_FN_RET_OK);
}

REG_FN(f_uart)
{
    bool uartEn = get_uartEn();
    if (uartEn && !val)
    {
        deca_uart_close();
    }
    else if (!uartEn && val)
    {
        deca_uart_init();
    }
    set_uartEn((uint8_t)(val) == 1);
    return (CMD_FN_RET_OK);
}

REG_FN(f_restore)
{
    CMD_ENTER_CRITICAL();
    restore_bssConfig();
    CMD_EXIT_CRITICAL();
    return (CMD_FN_RET_OK);
}


extern int test_mcps(int);

#ifdef LATER
REG_FN(f_test_mcps)
{
    char cmd[12];
    int mode = 0;
    sscanf(text, "%9s %d", cmd, &mode);

    test_mcps(mode);
    return (CMD_FN_RET_OK);
}
#endif
/**
 * @}
 */


/** @defgroup Various_commands
 * @brief    This commands to control the PDoA Node application;
 *           test commands;
 * @{
 */

REG_FN(f_decaid)
{
    hal_uwb.wakeup_with_io();

    diag_printf("Decawave device ID = 0x%08lx\r\n", dwt_readdevid());
    diag_printf("Decawave lotID = 0x%08lx, partID = 0x%08lx\r\n", dwt_getlotid(), dwt_getpartid());

    hal_uwb.sleep_enter();

    return (CMD_FN_RET_OK);
}


REG_FN(f_get_version)
{
    const char version[] = FULL_VERSION;
    diag_printf("VERSION:%s\r\n", version);
    return (CMD_FN_RET_OK);
}

/** @brief
 * */
REG_FN(f_decaJuniper)
{
    const char *ret = NULL;
    const char ver[] = FULL_VERSION;

    char *str = CMD_MALLOC(MAX_STR_SIZE);

    if (str)
    {
        extern const char ProjectName[];
        int hlen;

        hlen = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object

        sprintf(&str[strlen(str)], "{\"Info\":{\r\n");
        sprintf(&str[strlen(str)], "\"Device\":\"%s\",\n\r", (char *)ProjectName);
        sprintf(&str[strlen(str)], "\"Current App\":\"%s\",\r\n", AppGet()->app_name);

        sprintf(&str[strlen(str)], "\"Version\":\"%s\",\r\n", ver);

        sprintf(&str[strlen(str)], "\"Build\":\"%s %s\",\r\n", __DATE__, __TIME__);

        sprintf(&str[strlen(str)], "\"Apps\":[");

        /* Scan for known applications in the __known_command section*/
        extern uint32_t __known_commands_start;
        extern uint32_t __known_commands_end;

        command_t *knownApp;
        /* Mask to distinguish apps from other commands*/
        const uint32_t application_mask = mCmdGrp2;

        /* Loop and add the application to the output string*/
        for (knownApp = (command_t *)&__known_commands_start; knownApp < (command_t *)&__known_commands_end; knownApp++)
        {
            if (knownApp->mode & application_mask)
            {
                sprintf(&str[strlen(str)], "\"%s\",", knownApp->name);
            }
        }

        /* Remove last "," character*/
        if (str[strlen(str) - 1] == ',')
        {
            str[strlen(str) - 1] = '\0';
        }
        /* End the command braket*/
        sprintf(&str[strlen(str)], "],\r\n");

        sprintf(&str[strlen(str)], "\"Driver\":\"%s\"", dwt_version_string());

        #ifdef UWBSTACK
        sprintf(&str[strlen(str)],",\r\n\"UWB stack\":\"%s\"", UWBMAC_VERSION);
        #endif
        sprintf(&str[strlen(str)], "}}");

        sprintf(&str[2], "%04X", strlen(str) - hlen); // add formatted 4X of length, this will erase first '{'
        str[hlen] = '{';                              // restore the start bracket
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);
        ret = CMD_FN_RET_OK;
    }

    return (ret);
}


/**
 * @brief set or show current Key & IV parameters in JSON format
 * @param no param - show current Key & IV
 *        correct scanned string - set the params and then show them
 *        incorrect scanned string - error
 *
 * */
REG_FN(f_power)
{
    const char *ret = CMD_FN_RET_OK;

    char *str = CMD_MALLOC(MAX_STR_SIZE);

    if (str)
    {
        /* Display the Key Config */
        int n, hlen;

        unsigned int pwr, pgDly;
        int pgCnt;
        dwt_txconfig_t *txConfig = get_dwt_txconfig();

        n = sscanf(text, "%9s 0X%08x 0X%08x 0X%08x", str, &pwr, &pgDly, &pgCnt);

        if (n == 4)
        {
            txConfig->power = pwr;
            txConfig->PGdly = pgDly;
            txConfig->PGcount = pgCnt;
        }
        else if (n != 1)
        {
            ret = NULL;
        }

        hlen = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object
        sprintf(&str[strlen(str)], "{\"TX POWER\":{\r\n");

        sprintf(&str[strlen(str)], "\"PWR\":\"0x%08X\",\r\n", (unsigned int)txConfig->power);
        sprintf(&str[strlen(str)], "\"PGDLY\":\"0x%08X\",\r\n", (unsigned int)txConfig->PGdly);
        sprintf(&str[strlen(str)], "\"PGCOUNT\":\"0x%08X\"}}", (unsigned int)txConfig->PGcount);

        sprintf(&str[2], "%04X", strlen(str) - hlen); // add formatted 4X of length, this will erase first '{'
        str[hlen] = '{';                              // restore the start bracket
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);
    }
    return (ret);
}

/**
 * @brief sets or show the current Antenna Type
 * @param no param - show current antenna type
 *        correct scanned string - set the antenna type and then show it
 *        incorrect scanned string - error
 *
 * */
REG_FN(f_antenna)
{
    const char *ret = CMD_FN_RET_OK;
    char argv[NUMBER_OF_ANT_PORTS][12];

    int n;
    int arg_index, ant_index;
    bool bad_type;

    antenna_type_e *port;
    antenna_type_e antenna_type[NUMBER_OF_ANT_PORTS];

    rf_tuning_t *rf_tuning = get_rf_tuning_config();

    n = sscanf(text, "%*s %9s %9s %9s %9s", argv[0], argv[1], argv[2], argv[3]);
    diag_printf("\r\n"); // New line to start response after command

    if (n <= NUMBER_OF_ANT_PORTS)
    {
        /* Option to show possible values if the command is "VALUES" */
        if (strcmp(argv[0], "VALUES") == 0)
        {
            /* Print possible values for the "antenna" command */
            diag_printf("ANTENNA_TYPE POSSIBLE VALUES:\r\n");

            ant_index = 0;
            while (antenna_list[ant_index].name != NULL)
            {
                diag_printf("- %s\r\n", antenna_list[ant_index].name);
                ant_index++;
            }
            diag_printf("\r\n");
        }
        else if (n > 0)
        {
            /* Check values given for each antenna port */
            for (arg_index = 0; arg_index < n; arg_index++)
            {
                bad_type = true;
                ant_index = 0;

                while (antenna_list[ant_index].name != NULL)
                {
                    if (strcmp(argv[arg_index], antenna_list[ant_index].name) == 0)
                    {
                        antenna_type[arg_index] = antenna_list[ant_index].antenna_type;
                        bad_type = false;
                        break;
                    }
                    ant_index++;
                }

                if (bad_type)
                    break;
            }
            /* Check/report for invalid types... */
            if (bad_type)
            {
                diag_printf("INVALID ANTENNA_TYPE: %s\r\n", argv[arg_index]);
                ret = NULL;
            }
            else
            {
                /* ...if valid, change antenna types and take additional actions */
                port = &rf_tuning->antenna.port1;
                for (ant_index = 0; ant_index < n; ant_index++)
                {
                    *port = antenna_type[ant_index];
                    port++;
                }

                /* Additional action - Set Antenna Delay */
                // rf_tuning->antTx_a = (uint16_t)(0.5 * DEFAULT_ANTD);
                // rf_tuning->antRx_a = (uint16_t)(0.5 * DEFAULT_ANTD);
                // rf_tuning->antRx_b = (uint16_t)(0.5 * DEFAULT_ANTD);
            }
        }
    }
    else
    {
        diag_printf("INVALID COMMAND FORMAT!\r\n");
        ret = NULL;
    }

    /* Display Antenna Type */
    if (ret != NULL)
    {
        diag_printf("CURRENT ANTENNA_TYPE:\r\n");

        port = &rf_tuning->antenna.port1;
        for (ant_index = 0; ant_index < NUMBER_OF_ANT_PORTS; ant_index++)
        {
            diag_printf("PORT%d: %s\r\n", ant_index + 1, antenna_list[*port].name);
            port++;
        }
    }

    return (ret);
}

/**
 * @brief set or show current Key & IV parameters in JSON format
 * @param no param - show current Key & IV
 *        correct scanned string - set the params and then show them
 *        incorrect scanned string - error
 *
 * */
REG_FN(f_stskeyiv)
{
    const char *ret = CMD_FN_RET_OK;

    char *str = CMD_MALLOC(MAX_STR_SIZE);

    if (str)
    {
        /* Display the Key Config */
        int hlen, n;

        unsigned int key0, key1, key2, key3;
        unsigned int iv0, iv1, iv2, iv3;
        unsigned int sMode;

        n = sscanf(text, "%9s 0X%08x%08x%08x%08x 0X%08x%08x%08x%08x %d", str, &key3, &key2, &key1, &key0,
                   &iv3, &iv2, &iv1, &iv0, &sMode);
        sts_config_t *sts_config = get_sts_config();
        if (n == 9 || n == 10)
        {
            sts_config->stsInteropMode = sMode;

            sts_config->stsIv.iv0 = iv0;
            sts_config->stsIv.iv1 = iv1;
            sts_config->stsIv.iv2 = iv2;
            sts_config->stsIv.iv3 = iv3;

            sts_config->stsKey.key0 = key0;
            sts_config->stsKey.key1 = key1;
            sts_config->stsKey.key2 = key2;
            sts_config->stsKey.key3 = key3;
        }
        else if (n != 1)
        {
            ret = NULL;
        }

        hlen = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object
        sprintf(&str[strlen(str)], "{\"STS KEY_IV\":{\r\n");

        sprintf(&str[strlen(str)], "\"STS KEY\":\"0x%08X%08X%08X%08X\",\r\n",
                (unsigned int)sts_config->stsKey.key3,
                (unsigned int)sts_config->stsKey.key2,
                (unsigned int)sts_config->stsKey.key1,
                (unsigned int)sts_config->stsKey.key0);
        sprintf(&str[strlen(str)], "\"STS IV\":\"0x%08X%08X%08X%08X\",\r\n",
                (unsigned int)sts_config->stsIv.iv3,
                (unsigned int)sts_config->stsIv.iv2,
                (unsigned int)sts_config->stsIv.iv1,
                (unsigned int)sts_config->stsIv.iv0);
        sprintf(&str[strlen(str)], "\"STS_STATIC\":\"%d\"}}", (int)sts_config->stsInteropMode);

        sprintf(&str[2], "%04X", strlen(str) - hlen); // add formatted 4X of length, this will erase first '{'
        str[hlen] = '{';                              // restore the start bracket
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);
    }
    return (ret);
}

/**
 * @brief   set or show current UWB parameters in JSON format
 * @param if cmd "UWBCFG" has no params, then show the current UWB config
 *        if it has correctly scanned string - set the UWB config and then show them
 *        if it has incorrect scanned string - indicate an error (however no error check would
 *        be performed, so all incorrect parameters would be set to UWB settings)
 *
 * */
REG_FN(f_uwbcfg)
{
    const char *ret = CMD_FN_RET_OK;

    char *str = CMD_MALLOC(MAX_STR_SIZE);

    int n;
    int chan;           //!< Channel number (5 or 9)
    int txPreambLength; //!< DWT_PLEN_64..DWT_PLEN_4096
    int rxPAC;          //!< Acquisition Chunk Size (Relates to RX preamble length)
    int txCode;         //!< TX preamble code (the code configures the PRF, e.g. 9 -> PRF of 64 MHz)
    int rxCode;         //!< RX preamble code (the code configures the PRF, e.g. 9 -> PRF of 64 MHz)
    int sfdType;        //!< SFD type (0 for short IEEE 8-bit standard, 1 for DW 8-bit, 2 for DW 16-bit, 3 for 4z BPRF)
    int dataRate;       //!< Data rate {DWT_BR_850K or DWT_BR_6M8}
    int phrMode;        //!< PHR mode {0x0 - standard DWT_PHRMODE_STD, 0x3 - extended frames DWT_PHRMODE_EXT}
    int phrRate;        //!< PHR rate {0x0 - standard DWT_PHRRATE_STD, 0x1 - at datarate DWT_PHRRATE_DTA}
    int sfdTO;          //!< SFD timeout value (in symbols)
    int stsMode;        //!< STS mode (no STS, STS before PHR or STS after data)
    int stsLength;      //!< STS length (the allowed values are listed in dwt_sts_lengths_e
    int pdoaMode;       //!< PDOA mode

    if (str)
    {
        dwt_config_t *dwt_config = get_dwt_config();
        n = sscanf(text, "%9s %d %d %d %d %d %d %d %d %d %d %d %d %d", str, &chan, &txPreambLength, &rxPAC, &txCode,
                   &rxCode, &sfdType, &dataRate, &phrMode, &phrRate, &sfdTO, &stsMode, &stsLength, &pdoaMode);
        if (n == 14)
        { // set parameters :: this is unsafe, TODO :: add a range check
            dwt_config->chan = chan_to_deca(chan);
            dwt_config->txPreambLength = plen_to_deca(txPreambLength);
            dwt_config->rxPAC = pac_to_deca(rxPAC);
            dwt_config->dataRate = bitrate_to_deca(dataRate);
            dwt_config->stsLength = sts_length_to_deca(stsLength);

            dwt_config->txCode = txCode;
            dwt_config->rxCode = rxCode;
            dwt_config->sfdType = sfdType;
            dwt_config->phrMode = phrMode;
            dwt_config->phrRate = phrRate;
            dwt_config->sfdTO = sfdTO;
            dwt_config->stsMode = stsMode;
            dwt_config->pdoaMode = pdoaMode;
        }
        else if (n != 1)
        {
            ret = NULL; // produce an error
        }

        /* Display the UWB Config object */
        int hlen;

        hlen = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object
        sprintf(&str[strlen(str)], "{\"UWB PARAM\":{\r\n");

        sprintf(&str[strlen(str)], "\"CHAN\":%d,\r\n", deca_to_chan(dwt_config->chan));
        sprintf(&str[strlen(str)], "\"PLEN\":%d,\r\n", deca_to_plen(dwt_config->txPreambLength));
        sprintf(&str[strlen(str)], "\"PAC\":%d,\r\n", deca_to_pac(dwt_config->rxPAC));
        sprintf(&str[strlen(str)], "\"TXCODE\":%d,\r\n", dwt_config->txCode);
        sprintf(&str[strlen(str)], "\"RXCODE\":%d,\r\n", dwt_config->rxCode);
        sprintf(&str[strlen(str)], "\"SFDTYPE\":%d,\r\n", dwt_config->sfdType);
        sprintf(&str[strlen(str)], "\"DATARATE\":%d,\r\n", deca_to_bitrate(dwt_config->dataRate));
        sprintf(&str[strlen(str)], "\"PHRMODE\":%d,\r\n", dwt_config->phrMode);
        sprintf(&str[strlen(str)], "\"PHRRATE\":%d,\r\n", dwt_config->phrRate);
        sprintf(&str[strlen(str)], "\"SFDTO\":%d,\r\n", dwt_config->sfdTO);
        sprintf(&str[strlen(str)], "\"STSMODE\":%d,\r\n", dwt_config->stsMode);
        sprintf(&str[strlen(str)], "\"STSLEN\":%d,\r\n", deca_to_sts_length(dwt_config->stsLength));
        sprintf(&str[strlen(str)], "\"PDOAMODE\":%d}}", dwt_config->pdoaMode);


        sprintf(&str[2], "%04X", strlen(str) - hlen); // add formatted 4X of length, this will erase first '{'
        str[hlen] = '{';                              // restore the start bracket
        sprintf(&str[strlen(str)], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);
    }
    return (ret);
}

/**
 * @brief show current mode of operation,
 *           version, and the configuration
 *
 * */
REG_FN(f_stat)
{
    const char *ret = CMD_FN_RET_OK;

    char *str = CMD_MALLOC(MAX_STR_SIZE);

    if (str)
    {
        sprintf(str, "MODE: %s\r\n"
                     "LAST ERR CODE: %d\r\n"
                     "MAX MSG LEN: %d\r\n",
                AppGet()->app_name,
                AppGetLastError(),
                /*app.maxMsgLen*/ 0);

        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);
#ifdef LATER
        app.lastErrorCode = 0;
        app.maxMsgLen = 0;
#endif

        f_decaJuniper(NULL, pbss, 0, NULL);
        f_jstat(NULL, pbss, 0, NULL);
        f_get_discovered_list(NULL, NULL, 0, NULL);
        f_get_known_list(NULL, NULL, 0, NULL);
    }

    ret = CMD_FN_RET_OK;
    return (ret);
}

static int print_help_line(char *str, int cnt, const command_t *known_command)
{
    switch (known_command->mode & mCmdGrpMASK)
    {
    case mCmdGrp0:
        if (cnt > 0)
        {
            sprintf(&str[cnt], "\r\n");
            port_tx_msg((uint8_t *)str, strlen(str));
            cnt = 0;
        }
        /*print the Group name */
        sprintf(str, "---- %s---\r\n", known_command->cmnt);
        port_tx_msg((uint8_t *)str, strlen(str));
        break;

    case mCmdGrp1:
    case mCmdGrp2:
        /* print appropriate list of parameters for the current application */
        if (known_command->name)
        {
            sprintf(&str[cnt], "%-10s", known_command->name);
            cnt += CMD_COLUMN_WIDTH;
            if (cnt >= CMD_COLUMN_WIDTH * CMD_COLUMN_MAX)
            {
                sprintf(&str[cnt], "\r\n");
                port_tx_msg((uint8_t *)str, strlen(str));
                cnt = 0;
            }
        }
        break;

    case mCmdGrp3:
    case mCmdGrp4:
    case mCmdGrp5:
    case mCmdGrp6:
    case mCmdGrp7:
    case mCmdGrp8:
    case mCmdGrp9:
    case mCmdGrp10:
    case mCmdGrp11:
    case mCmdGrp12:
    case mCmdGrp13:
    case mCmdGrp14:
        /*reserved for the future*/
    default:
        break;
    }
    return cnt;
}

/**
 * @brief Show all available commands
 *
 * */
REG_FN(f_help_std)
{
    int cnt = 0;
    const char * ret = NULL;
    char *str = CMD_MALLOC(MAX_STR_SIZE);

    extern uint32_t __known_commands_start;
    extern uint32_t __known_commands_end;
    extern uint32_t __known_commands_app_start;
    extern uint32_t __known_commands_ilde_start;
    extern uint32_t __known_commands_service_start;

    known_commands = (command_t *)&__known_commands_start;

    if(str)
    {
        CMD_ENTER_CRITICAL();

        extern const char ProjectName[];
        reporter_instance.print((char *)ProjectName, strlen(ProjectName));
        reporter_instance.print("\r\n", 2);

        /* Scan for known applications in the __known_command section*/
        for(known_commands = (command_t*)&__known_commands_start; known_commands < (command_t*)&__known_commands_end; known_commands++)
        {
            uint32_t mode = known_commands->mode;

            if ((mode & mMASK) == mANY || mIDLE == AppGet()->app_mode)
            {
                if(known_commands == (command_t*)&__known_commands_start)
                {
                    cnt = print_help_line(str, cnt, known_commands_anytime);
                }
                else if(known_commands == (command_t*)&__known_commands_app_start)
                {
                    cnt = print_help_line(str, cnt, known_commands_app_start);
                }
                else if(known_commands == (command_t*)&__known_commands_ilde_start)
                {
                    cnt = print_help_line(str, cnt, known_commands_idle);
                }
                else if(known_commands == (command_t*)&__known_commands_service_start)
                {
                    cnt = print_help_line(str, cnt, known_commands_service);
                }
                cnt = print_help_line(str, cnt, known_commands);
            }
        }
        if (((AppGet()->app_mode & mAPPMASK) == mAPP) && (AppGet()->sub_command != NULL))
        {
            const command_t *sub_cmd = AppGet()->sub_command;
            while (1)
            {
                cnt = print_help_line(str, cnt, sub_cmd);
                if (sub_cmd->mode & APP_LAST_SUB_CMD)
                {
                    break;
                }
                sub_cmd++;
            }
        }
        sprintf(&str[cnt], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_EXIT_CRITICAL();

        CMD_FREE(str);
        ret = CMD_FN_RET_OK;
    }

    return (ret);
}


/*
 * @brief This fn() displays the help information to the function,
 *        i.e. comment field.
 *        usage: "help help", "help tag" etc
 *
 * */
REG_FN(f_help_help)
{
    int indx = 0, n = 0;
    const char *ret = NULL;
    char *str = CMD_MALLOC(MAX_STR_SIZE);

    if (str)
    {
        CMD_ENTER_CRITICAL();

        while (known_commands[indx].cmnt != NULL)
        {
            uint32_t mode = known_commands[indx].mode;

            if ((mode & mMASK) == AppGet()->app_mode || (mode & mMASK) == mANY || mIDLE == AppGet()->app_mode)
            {
                if ((known_commands[indx].name != NULL) && (strcmp(known_commands[indx].name, text) == 0))
                {
                    sprintf(str, "\r\n%s:\r\n", text);
                    reporter_instance.print((char *)str, strlen(str));

                    const char *ptr = known_commands[indx].cmnt;

                    bool auto_crlf = (strchr(ptr, '\r') != NULL);
                    if (auto_crlf)
                    {
                        reporter_instance.print((char *)ptr, strlen(ptr));
                    }
                    else
                    {
                        while (ptr)
                        {
                            // CMD_COLUMN_WIDTH*CMD_COLUMN_MAX
                            n = snprintf(str, 78, "%s", ptr);
                            sprintf(&str[strlen(str)], "\r\n");
                            reporter_instance.print((char *)str, strlen(str));
                            if (n < 78)
                            {
                                ptr = NULL;
                            }
                            else
                            {
                                ptr += 77;
                            }
                        }
                    }

                    break;
                }
            }

            indx++;
        }

        sprintf(str, "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        CMD_EXIT_CRITICAL();

        CMD_FREE(str);
        ret = CMD_FN_RET_OK;
    }

    return ret;
}

/**
 * @brief Show all available commands
 *
 * */
REG_FN(f_help_app)
{
    char help[12];
    char cmd[12];
    int n;
    const char *ret = NULL;

    n = sscanf(text, "%9s %10s", cmd, help);

    switch (n)
    {
    case 1:
        ret = f_help_std(cmd, pbss, val, params);
        break;
    case 2:
        if (help[0] == 0)
        {
            ret = f_help_help(&help[1], pbss, val, params);
        }
        else
        {
            ret = f_help_help(&help[0], pbss, val, params);
        }
        break;
    default:
        break;
    }

    return (ret);
}

//-----------------------------------------------------------------------------
// Communication change section

/**
 * @brief save configuration
 *
 * */
REG_FN(f_save)
{
    error_e err_code;

    CMD_ENTER_CRITICAL();

    if (AppGet()->app_mode & APP_SAVEABLE)
    {
        err_code = AppSetDefaultEvent(AppGet());
    }
    else
    {
        err_code = AppSetDefaultEvent(&idle_app[0]); // default app is defined in the main();
    }

    CMD_EXIT_CRITICAL();

    if (err_code != _NO_ERR)
    {
        error_handler(0, err_code); // not a fatal error
        return (NULL);
    }

    return (CMD_FN_RET_OK);
}

/**
 * @}
 */


//-----------------------------------------------------------------------------

/** end f_xx command functions */

const char STD_CMD_COMMENT         []={"This command described is in the documentation"};


const char COMMENT_STOP[] = {"Stops running any top-level applications"};
const char COMMENT_STAT[] = {"Displays the Status information"};
const char COMMENT_SAVE[] = {"Saves the configuration to the NVM"};
const char COMMENT_DECAJUNIPER[] = {"This command reports the running application and the version information"};
const char COMMENT_HELP[] = {"This command displays the help information.\r\nUsage: \"HELP\" or \"HELP <CMD>\", <CMD> is the command from the list, i.e. \"HELP SAVE\"."};

const char COMMENT_UART[] = {"Usage: To initialize selected UART \"UART <DEC>\""};

const char COMMENT_RESTORE[] = {"Restores the default configuration, both UWB and System."};
const char COMMENT_DIAG[] = {"Diagnostic mode: Display of complementary information during ranging.\r\nUsage: \"DIAG <DEC>\" (0:OFF, 1:ON)"};

const char COMMENT_UWBCFG[] = {"UWB configuration\r\nUsage: To see UWB parameters \"UWBCFG\". To set the UWB config, list the parameters as a string argument \"UWBCFG <List of parameters>\""};
const char COMMENT_STSKEYIV[] = {"Sets STS Key, IV and their behavior mode.\r\nUsage: To see STS KEY, IV and Mode \"STSKEYIV\". To set\"STSKEYIV 0x<STS_KEY_HEX_16> 0x<IV_HEX_16> <MODE_DEC>\".\r\n<MODE_DEC>: 1 use fixed STS (Default), 0 use dynamic STS"};
const char COMMENT_TXPOWER[] = {"Tx Power settings.\r\nUsage: To see Tx power \"TXPOWER\". To set the Tx power \"TXPOWER 0x<POWER_HEX> 0x<PGDLY_HEX> 0x<PGCOUNT_HEX>\""};
const char COMMENT_ANTENNA[] = {"Sets Antenna Type.\r\nUsage: To see Antenna \"ANTENNA\". To set the current antenna type for each port \"ANTENNA <PORT1> <PORT2>...\". To see possible values \"antenna values\"."};

const char COMMENT_THREAD[] = {"Displays Heap and Threads stack usage"};

const char COMMENT_DECAID[] = {"Displays UWB chip information"};
const char COMMENT_VERSION[] = {"Shows version of the SW"};

command_t *known_commands;

//-----------------------------------------------------------------------------
/** list of known commands:
 * NAME, allowed_MODE,     REG_FN(fn_name)
 * */
const struct command_s known_commands_anytime_all [] __attribute__((section(".known_commands_anytime")))= {
    /** CMDNAME   MODE   fn  comment */
    /** Anytime commands */
    {"HELP",    mCmdGrp1 | mANY,   f_help_app,              COMMENT_HELP },
    {"?",       mCmdGrp1 | mANY,   f_help_app,              COMMENT_HELP },
    {"STOP",    mCmdGrp1 | mANY,   f_stop,                  COMMENT_STOP },
    {"THREAD",  mCmdGrp1 | mANY,   f_thread,                COMMENT_THREAD },
    {"STAT",    mCmdGrp1 | mANY,   f_stat,                  COMMENT_STAT },
    {"SAVE",    mCmdGrp1 | mANY,   f_save,                  COMMENT_SAVE },
    {"DECA$",   mCmdGrp1 | mANY,   f_decaJuniper,           COMMENT_DECAJUNIPER },
};

const struct command_s known_commands_idle_uart [] __attribute__((section(".known_commands_ilde")))= {
    {"UART",    mCmdGrp1 | mIDLE,  f_uart,                  COMMENT_UART}, 
};

const struct command_s known_commands_service_all [] __attribute__((section(".known_commands_service")))= {
    /** 5. service commands */
    {"RESTORE", mCmdGrp1 | mIDLE,  f_restore,               COMMENT_RESTORE},
    {"DIAG",    mCmdGrp1 | mIDLE,  f_diag,                  COMMENT_DIAG},

    {"UWBCFG",  mCmdGrp1 | mIDLE,  f_uwbcfg,                COMMENT_UWBCFG},
    {"STSKEYIV",mCmdGrp1 | mIDLE,  f_stskeyiv,              COMMENT_STSKEYIV},
    {"TXPOWER", mCmdGrp1 | mIDLE,  f_power,                 COMMENT_TXPOWER},
    {"ANTENNA", mCmdGrp1 | mIDLE,  f_antenna,               COMMENT_ANTENNA},
    {"DECAID",  mCmdGrp1 | mIDLE,  f_decaid,                COMMENT_DECAID},
    {"VERSION", mCmdGrp1 | mIDLE,  f_get_version,           COMMENT_VERSION},
#ifdef LATER
    {"MCPS",    mCmdGrp1 | mIDLE,  f_test_mcps,             STD_CMD_COMMENT},
#endif
};
