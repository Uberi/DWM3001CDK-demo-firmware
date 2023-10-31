/**
 * @file      uci_transport.c
 *
 * @brief     uci transport functions
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

#include <stdlib.h>
#include <linux/errno.h>
#include "reporter.h"
#include "cmsis_os.h"
#include "HAL_timer.h"
/* uci_transport needs to be after cmsis_os for redefiniton of ASSERT issue */
#include "uci_transport.h"

#ifdef USE_UCI_HSSPI
#include "HAL_hsspi.h"
#endif

#ifdef USE_UCI_UART_1
#include "HAL_uci_uart.h"
#endif

#define UCI_GARBAGE_TIMEOUT_MS 100

/* nordic_common.h is not protecting MIN / MAX definition */
#include "minmax.h"

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - __builtin_offsetof(type, member)))

static struct uci_blk *rx = NULL;
static uint16_t rx_offset = 0;
static uint32_t tmr_rd;

void uci_tp_attach(struct uci_transport *tr, struct uci *uci)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);

    s->uci = uci;
    Timer.start(&tmr_rd);
}

void uci_tp_detach(struct uci_transport *tr)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);

    if (rx)
        uci_blk_free_all(s->uci, rx);

    rx_offset = 0;
    rx = NULL;

    s->uci = NULL;
}

void uci_tp_usb_packet_send_ready(struct uci_transport *tr)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);
    struct uci_blk *p;

    while ((p = uci_packet_send_get_ready(s->uci)))
    {
        reporter_instance.print((char *)p->data, p->len);
        uci_packet_send_done(s->uci, p, 0);
    }
}

#ifdef USE_UCI_HSSPI

#define UCI_NO_RESPONSE_TIMEOUT_MS 1000
static uint32_t tmr_wr;
void uci_tp_hsspi_packet_send_ready(struct uci_transport *tr)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);
    struct uci_blk *p;
    bool done;

    while ((p = uci_packet_send_get_ready(s->uci)))
    {
        hsspi_write(p->data, p->len);
        Timer.start(&tmr_wr);
        done = false;
        while (!done)
        {
            if (hsspi_data_sent())
            {
                done = true;
                uci_packet_send_done(s->uci, p, 0);
            }
            else
            {
                done = Timer.check(tmr_wr, UCI_NO_RESPONSE_TIMEOUT_MS);
            }
        }

        hsspi_read((uint8_t *)s->read_buf, UCI_MAX_PACKET_SIZE);
    }
}
#endif

#ifdef USE_UCI_UART_1
void uci_tp_uart1_packet_send_ready(struct uci_transport *tr)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);
    struct uci_blk *p;

    while ((p = uci_packet_send_get_ready(s->uci)))
    {
        uci_uart_transmit(p->data, p->len);
        uci_packet_send_done(s->uci, p, 0);
    }
}
#endif


/* Platform agnostic read entry point for the UCI transport read
 *  @brief This function process the buffer provided in entry and
 *  check if it contains a valid UCI packet.
 *  If it does -> call UCI-CORE
 *  If it does not -> return NO_PACKET
 *  @args tr : transport structure containing buffer to read from, buffer len and uci structure
 *  @return n : number of byte read from the input buffer
 */

int uci_tp_read(struct uci_tp *tr)
{
    uint16_t n = 0;
    uint16_t data_size = *(tr->p_read_buf_size);
    uint16_t available = data_size;
    char *buf = tr->read_buf;
    bool garbage_present = false;

    while (available > 0)
    {
        if (!rx)
        {
            rx = uci_blk_alloc(tr->uci, UCI_MAX_PACKET_SIZE);
            if (!rx)
                return -ENOMEM;
        }

        if (rx_offset < UCI_PACKET_HEADER_SIZE)
            n = MIN(UCI_PACKET_HEADER_SIZE - rx_offset, available);
        else
            n = MIN(UCI_PACKET_HEADER_SIZE + rx->data[3] - rx_offset, available);

        memcpy(rx->data + rx_offset, buf, n);

        buf += n;
        rx_offset += n;
        available -= n;

        if (rx_offset > 0)
        {
            garbage_present = Timer.check(tmr_rd, UCI_GARBAGE_TIMEOUT_MS);
            Timer.start(&tmr_rd);
        }

        if (rx_offset >= UCI_PACKET_HEADER_SIZE)
        {
            if (rx->data[3] > UCI_MAX_PAYLOAD_SIZE)
            {
                uci_tp_flush(tr);
                return -EINVAL;
            }
            if (rx_offset == (UCI_PACKET_HEADER_SIZE + rx->data[3]))
            {
                rx->len = rx_offset;
                uci_packet_recv(tr->uci, rx);
                rx = NULL;
                rx_offset = 0;
            }
        }
        else
        {
#ifndef USE_UCI_UART_1
            if (garbage_present)
            {
                uci_tp_flush(tr);
                return -EINVAL;
            }
#endif
        }
    }

    return data_size;
}

void uci_tp_flush(struct uci_tp *tr)
{
    uci_blk_free_all(tr->uci, rx);
    rx = NULL;
    rx_offset = 0;
}