/**
 * @file      usb_uart_rx.c
 *
 * @brief     This file supports Decawave USB-TO-SPI and Control modes.
 *            Functions can be used in both bare-metal and RTOS implementations.
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

/* Includes */
#include <string.h>
#include <stdint.h>

#include "usb_uart_rx.h"
#include "deca_error.h"

#include "app.h"
#include "comm_config.h"
#include "controlTask.h"
//#include "usb2spi.h"
#include "cmd.h"
#include "usb_uart_tx.h"
#ifdef USB_ENABLE
#include "HAL_usb.h"
#endif
#ifdef BT_UART_ENABLE
#include "HAL_bt_uart.h"
#endif
#include "task_signal.h"
#include "InterfUsb.h"

/* For bare-metal implementation this critical defines may be required. */
#define USB_UART_ENTER_CRITICAL()
#define USB_UART_EXIT_CRITICAL()

#define MAX_CMD_LENGTH 0x100

extern data_circ_buf_t *uartRx;
extern data_circ_buf_t *usbRx;
extern data_circ_buf_t *btRx;

/* Receiving command type status */
typedef enum
{
    cmdREGULAR = 0, /* Regular command */
    cmdJSON,        /* JSON command */
    cmdUNKNOWN_TYPE /* Unknown command */
} command_type_e;

//-----------------------------------------------------------------------------
// IMPLEMENTATION

/*
 * @brief    Waits only commands from incoming stream.
 *             The binary interface (deca_usb2spi stream) is not allowed.
 *
 * @return  COMMAND_READY : the data for future processing can be found in app.local_buff : app.local_buff_len
 *          NO_DATA : no command yet
 */
usb_data_e waitForCommand(uint8_t *pBuf, uint16_t len, uint16_t *read_offset, uint16_t cyclic_size)
{
    usb_data_e ret = NO_DATA;
    static uint8_t cmdLen = 0;
    static uint8_t cmdBuf[MAX_CMD_LENGTH]; /* Commands buffer */
    uint16_t cnt;
    static command_type_e command_type = cmdUNKNOWN_TYPE;
    static uint8_t brackets_cnt;

    uint16_t local_buff_length = 0;

    for (cnt = 0; cnt < len; cnt++) // Loop over the buffer rx data
    {
        if (pBuf[*read_offset] == '\b') // erase of a char in the terminal
        {
            port_tx_msg((uint8_t *)"\b\x20\b", 3);
            if (cmdLen)
            {
                cmdLen--;
            }
        }
        else
        {
            port_tx_msg(&pBuf[*read_offset], 1);
            if (pBuf[*read_offset] == '\n' || pBuf[*read_offset] == '\r')
            {
                if ((cmdLen!=0)&&(command_type==cmdREGULAR))//Checks if need to handle regular command
                {//Update the app commands buffer
                    memcpy(&local_buff[local_buff_length],cmdBuf,cmdLen);
                    local_buff[local_buff_length+cmdLen]='\n';
                    local_buff_length+=(cmdLen+1);
                    cmdLen=0;
                    command_type=cmdUNKNOWN_TYPE;
                    ret = COMMAND_READY;
                }
            }
            else if (command_type==cmdUNKNOWN_TYPE)
            {//Find out if getting regular command or JSON
                cmdBuf[cmdLen]=pBuf[*read_offset];
                if (pBuf[*read_offset]== '{')
                {//Start Json command
                    command_type=cmdJSON;
                    brackets_cnt=1;
                }
                else
                { // Start regular command
                    command_type = cmdREGULAR;
                }
                cmdLen++;
            }
            else if (command_type == cmdREGULAR)
            { // Regular command
                cmdBuf[cmdLen] = pBuf[*read_offset];
                cmdLen++;
            }
            else
            { // Json command
                cmdBuf[cmdLen] = pBuf[*read_offset];
                cmdLen++;
                if (pBuf[*read_offset] == '{')
                {
                    brackets_cnt++;
                }
                else if (pBuf[*read_offset] == '}')
                {
                    brackets_cnt--;
                    if (brackets_cnt==0)//Got a full Json command
                    {//Update the app commands buffer
                        memcpy(&local_buff[local_buff_length],cmdBuf,cmdLen);
                        local_buff[local_buff_length+cmdLen]='\n';
                        local_buff_length+=(cmdLen+1);
                        cmdLen=0;
                        command_type=cmdUNKNOWN_TYPE;
                        ret = COMMAND_READY;
                    }
                }
            }
        }
        *read_offset = (*read_offset + 1) & cyclic_size;
        if (cmdLen >= sizeof(cmdBuf)) /* Checks if command too long and we need to reset it */
        {
            cmdLen = 0;
            command_type = cmdUNKNOWN_TYPE;
        }
    }

    if (ret == COMMAND_READY)
    { // If there is at least 1 command, add 0 at the end
        local_buff[local_buff_length] = 0;
        local_buff_length++;
    }
    return (ret);
}


/* @fn     usb_uart_rx
 * @brief  this should be calling on a reception of a data from UART or USB.
 *         uses platform-dependent
 *
 * */
usb_data_e usb_uart_rx(void)
{
    usb_data_e ret = NO_DATA;
    uint16_t uartLen, headUart, tailUart;
#ifdef USB_ENABLE
    uint16_t usbLen, headUsb, tailUsb;
#endif
#ifdef BT_UART_ENABLE
    uint16_t btLen, headBt, tailBt;
#endif
    /* USART control prevails over USB control if both at the same time */

    USB_UART_ENTER_CRITICAL();
    headUart = uartRx->head;
#ifdef USB_ENABLE
    headUsb = usbRx->head;
#endif
#ifdef BT_UART_ENABLE
    headBt = btRx->head;
#endif

    USB_UART_EXIT_CRITICAL();

    tailUart = uartRx->tail;
    uartLen = CIRC_CNT(headUart, tailUart, sizeof(uartRx->buf));
#ifdef USB_ENABLE
    tailUsb = usbRx->tail;
    usbLen = CIRC_CNT(headUsb, tailUsb, sizeof(usbRx->buf));
#endif
#ifdef BT_UART_ENABLE
    tailBt = btRx->tail;
    btLen = CIRC_CNT(headBt, tailBt, sizeof(btRx->buf));
#endif

    if ((uartLen > 0) && get_uartEn())
    {
        ret = AppGet()->on_rx(uartRx->buf, uartLen, &tailUart, sizeof(uartRx->buf) - 1);
        USB_UART_ENTER_CRITICAL();
        uartRx->tail = tailUart;
        USB_UART_EXIT_CRITICAL();
    }
#ifdef USB_ENABLE
    else if ((usbLen > 0) && (UsbGetState() == USB_CONFIGURED))
    {
        ret = AppGet()->on_rx(usbRx->buf, usbLen, &tailUsb, sizeof(usbRx->buf) - 1);
        USB_UART_ENTER_CRITICAL();
        usbRx->tail = tailUsb;
        USB_UART_EXIT_CRITICAL();
    }
#endif
#ifdef BT_UART_ENABLE
    if (btLen > 0)
    {
        ret = AppGet()->on_rx(btRx->buf, btLen, &tailBt, sizeof(btRx->buf) - 1);
        USB_UART_ENTER_CRITICAL();
        btRx->tail = tailBt;
        USB_UART_EXIT_CRITICAL();
    }
#endif
    return ret;
}
