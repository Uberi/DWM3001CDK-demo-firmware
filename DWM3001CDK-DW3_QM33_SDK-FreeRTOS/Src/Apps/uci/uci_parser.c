/**
 * @file      uci_parser.c
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

#include <ctype.h>
#include <string.h>
#include "usb_uart_rx.h"
#include "circular_buffers.h"
#include "uci_parser.h"
#include "controlTask.h"

/* @fn        check_for_stop
 * @brief     check if the stop command has been received
 * @return    DATA_STOP if stop command was received
 *            DATA_READY if byte stream does not contain stop
 * */
static const char dwSTOP[] = "STOP";
static const char dwSAVE[] = "SAVE";
static char last[5] = {0};

static usb_data_e check_for_stop(uint8_t *p, uint16_t len)
{
    for (int i = 0; i < len; i++)
    {
        last[0] = last[1];
        last[1] = last[2];
        last[2] = last[3];
        last[3] = toupper(p[i]);
        if (strcmp(last, dwSTOP) == 0)
        {
            return DATA_STOP;
        }
        if (strcmp(last, dwSAVE) == 0)
        {
            return DATA_SAVE;
        }
    }
    return DATA_READY;
}

/*
 * @brief    serialise circular buffer before calling the function processing the buffer
 * 			data
 *
 * @return  data_ready : the data is ready for future processing in usb2spi or uci application
 *                       data can be found in app.local_buff : app.local_buff_len
 * 			stop       : stop command was detected and will exit UCI mode
 *          no_data    : no valid data yet
 */
usb_data_e cir_to_ser_buf(uint8_t *pbuf, uint16_t len, uint16_t *read_offset, uint16_t cyclic_size)
{
    uint16_t datalen = 0;
    usb_data_e ret;
    uint16_t cnt;

    ret = NO_DATA;

    /* wait for valid usb2spi message from pbuf */
    if ((len + datalen) < sizeof(local_buff) - 1)
    {
        for (cnt = 0; cnt < len; cnt++)
        {
            local_buff[datalen] = pbuf[*read_offset];
            *read_offset = (*read_offset + 1) & cyclic_size;
            datalen++;
        }
        local_buff_length = datalen;
        ret = check_for_stop(local_buff, datalen);
    }
    else
    { /* overflow in usb2spi protocol : flush everything */
        datalen = 0;
    }

    return (ret);
}
