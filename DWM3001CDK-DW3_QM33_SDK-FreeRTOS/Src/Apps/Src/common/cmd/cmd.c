/**
 * @file      cmd.c
 *
 * @brief     Command string as specified in document SWxxxx version X.x.x
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

#include "cmd.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "appConfig.h"
#include "cJSON.h"
#include "cmd_fn.h"
#include "reporter.h"
#include "usb_uart_tx.h"


/*
 *    Command interface
 */


/* We want cJSON to use a definitive malloc/free */
static const cJSON_Hooks sCmdcJSON_hooks = {
    .malloc_fn = CMD_MALLOC,
    .free_fn = CMD_FREE};


/* IMPLEMENTATION */
void command_parser_init(void)
{
    cJSON_InitHooks((cJSON_Hooks *)&sCmdcJSON_hooks);
}

/*
 * @brief "error" will be sent if error during parser or command execution returned error
 * */
static void cmd_onERROR(const char *err)
{
    char *str = CMD_MALLOC(MAX_STR_SIZE);

    if (str)
    {
        strcpy(str, "error \r\n");
        if (strlen(err) < (MAX_STR_SIZE - 6 - 3 - 1))
        {
            strcpy(&str[6], err);
            strcpy(&str[6 + strlen(err)], "\r\n");
        }
        reporter_instance.print((char *)str, strlen(str));

        CMD_FREE(str);
    }
}


/* @fn      command_parser
 * @brief   checks if input "text" string in known "COMMAND" or "PARAMETER VALUE" format,
 *          checks their execution permissions, a VALUE range if restrictions and
 *          executes COMMAND or sets the PARAMETER to the VALUE
 * */
void command_parser(usb_data_e res, char *text)
{
    char *temp_str = text;
    command_e equal;
    int val;
    cJSON *json_root, *json_params;
    char cmd[20];
    const char *ret;

    extern uint32_t __known_commands_start;
    extern uint32_t __known_commands_end;

    if (res != COMMAND_READY)
        return;

    known_commands = (command_t *)&__known_commands_start;

    while (*temp_str)
    {
        *temp_str = (char)toupper(*temp_str);
        temp_str++;
    }

    /* Assume text may have more than one command inside.
     * For example "getKLIST\nnode 0\n" : this will execute 2 commands.
     * */
    text = strtok(text, "\n"); // get first token

    while (text != NULL)
    {
        equal = _NO_COMMAND;
        json_params = NULL;
        json_root = NULL;
        cmd[0] = 0; // Initialize no command

        if (*text == '{')
        { // Probably a Json command
            json_root = cJSON_Parse(text);
            if (json_root != NULL)
            {                                                                     // Got valid Json command
                temp_str = cJSON_GetObjectItem(json_root, CMD_NAME)->valuestring; // Get command name
                if (temp_str != NULL)
                {                                                             // Got right command name
                    json_params = cJSON_GetObjectItem(json_root, CMD_PARAMS); // Get command params
                    if (json_params != NULL)
                    { // We have a Json so we need to update command.
                        sscanf(temp_str, "%9s", cmd);
                    }
                }
            }
        }
        else
        { // It is not a Json command
            sscanf(text, "%9s %d", cmd, &val);
        }


        /* Scan for known applications in the __known_command section*/
        for(known_commands = (command_t*)&__known_commands_start; known_commands < (command_t*)&__known_commands_end; known_commands++)
        {
            if (known_commands->name && strcmp(cmd, known_commands->name) == 0)
            {
                equal = _COMMAND_FOUND;

                /* Check the command mode. to define the execution permission*/
                uint32_t mode = known_commands->mode & mMASK;

                switch (mode)
                {
                /* If it is an anytime command then launch it */
                case mANY:
                    equal = _COMMAND_ALLOWED;
                    break;
                /* If it is an app then check the current running app mode */
                case mIDLE:
                    if (mode == AppGet()->app_mode)
                    {
                        equal = _COMMAND_ALLOWED;
                    }
                    break;
                /* If it is a subcommand then check is done later*/
                case mAPP:
                    break;

                default:
                    break;
                }
                /* At this stage the command is not allowed to execute
                Check if the command is a sub command of the running application*/
                const struct command_s *sub_cmd = AppGet()->sub_command;
                if (sub_cmd != NULL)
                {
                    while (equal != _COMMAND_ALLOWED)
                    {
                        if (sub_cmd == known_commands)
                        {
                            /* The command is a subcommand of the running app*/
                            equal = _COMMAND_ALLOWED;
                            break;
                        }
                        if (sub_cmd->mode & APP_LAST_SUB_CMD)
                        {
                            /* Check if it is the last */
                            break;
                        }
                        sub_cmd++;
                    }
                }
                break;
            }
        }

        switch (equal)
        {
        case (_COMMAND_FOUND): {
            cmd_onERROR(" incompatible mode");
            break;
        }
        case (_COMMAND_ALLOWED): {
            /* execute corresponded fn() */
            ret = known_commands->fn(text, NULL, val,json_params);

            if (ret)
            {
                reporter_instance.print((char *)ret, strlen(ret));
            }
            else
            {
                cmd_onERROR(" function");
            }
            break;
        }
        default:
            break;
        }

        if (json_root != NULL)
        {
            cJSON_Delete(json_root);
        }

        text = strtok(NULL, "\n");
    }
}


/* end of cmd.c */
