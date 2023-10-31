/**
 * @file      usb_uart_tx.c
 *
 * @brief     Puts message to circular buffer which will be transmitted by flushing thread
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

#include "usb_uart_tx.h"
#include "circular_buffers.h"
#include "HAL_lock.h"
#include "HAL_uart.h"
#ifdef USB_ENABLE
#include "HAL_usb.h"
#include "InterfUsb.h"
#endif
#ifdef BT_UART_ENABLE
#include "HAL_bt_uart.h"
#endif
#include "HAL_error.h"
#include "HAL_timer.h"
#include "app.h"
#include "flushTask.h"

#include "minmax.h"
#include "comm_config.h"


//-----------------------------------------------------------------------------
//     USB/UART report section

// the size is such long because of possible ACCUMULATORS sending
#ifdef TINY_BUILD
#define USB_REPORT_BUFSIZE 0x800 /**< the size of report buffer, must be 1<<N, i.e. 0x800,0x1000 etc*/
#else
#define USB_REPORT_BUFSIZE 0x8000 /**< the size of report buffer, must be 1<<N, i.e. 0x800,0x1000 etc*/
#endif
#define USB_UART_TX_TIMEOUT_MS ((USB_REPORT_BUFSIZE * 10) / 115) /**< (USB_REPORT_BUFSIZE * (8+2) / 115) = 1400 */ /**< the max timeout to output of the whole report buffer on UART speed 115200, ms */
#ifdef BT_UART_ENABLE
#define CDC_DATA_FS_MAX_PACKET_SIZE 62 /* Reduced from 64 bytes to 62 bytes to reuse the buffer for BLE */
#else
#define CDC_DATA_FS_MAX_PACKET_SIZE 64
#endif

static uint8_t ubuf[CDC_DATA_FS_MAX_PACKET_SIZE]; /**< linear buffer, to transmit next chunk of data */

static struct _txHandle
{
    dw_hal_lockTypeDef lock; /**< locking object  */
    struct
    {
        uint16_t head;
        uint16_t tail;
        uint8_t buf[USB_REPORT_BUFSIZE]; /**< Large USB/UART circular Tx buffer */
    } Report;                            /**< circular report buffer, data to transmit */
}

txHandle = {
    .lock = DW_HAL_NODE_UNLOCKED,
    .Report.head = 0,
    .Report.tail = 0
};


//-----------------------------------------------------------------------------
// Implementation

int reset_report_buf(void)
{
    QHAL_LOCK(&txHandle);
    txHandle.Report.head = txHandle.Report.tail = 0;
    QHAL_UNLOCK(&txHandle);
    return _NO_ERR;
}

/* @fn         copy_tx_msg()
 * @brief     put message to circular report buffer
 *             it will be transmitted in background ASAP from flushing thread
 * @return    HAL_BUSY - can not do it now, wait for release
 *             HAL_ERROR- buffer overflow
 *             HAL_OK   - scheduled for transmission
 * */
error_e copy_tx_msg(uint8_t *str, int len)
{
    error_e ret = _NO_ERR;
    uint16_t head, tail;
    const uint16_t size = sizeof(txHandle.Report.buf) / sizeof(txHandle.Report.buf[0]);

    /* add TX msg to circular buffer and increase circular buffer length */

    QHAL_LOCK(&txHandle); //"return HAL_BUSY;" if locked
    head = txHandle.Report.head;
    tail = txHandle.Report.tail;

    if (CIRC_SPACE(head, tail, size) > len)
    {
        while (len > 0)
        {
            txHandle.Report.buf[head] = *(str++);
            head = (head + 1) & (size - 1);
            len--;
        }

        txHandle.Report.head = head;
    }
    else
    {
        /* if packet can not fit, setup TX Buffer overflow ERROR and exit */
        error_handler(0, _ERR_TxBuf_Overflow);
        ret = _ERR_TxBuf_Overflow;
    }

    QHAL_UNLOCK(&txHandle);
    return ret;
}

/* @fn        port_tx_msg
 * @brief     wrap for copy_tx_msg
 *             Puts message to circular report buffer
 *
 * @return    see copy_tx_msg()
 * */
error_e port_tx_msg(uint8_t *str, int len)
{
    error_e ret = copy_tx_msg(str, len);
    NotifyFlushTask();
    return (ret);
}


//-----------------------------------------------------------------------------
//     USB/UART report : platform - dependent section
//                      can be in platform port file


/* @fn        flush_report_buff()
 * @brief    FLUSH should have higher priority than reporter_instance.print()
 *             This shall be called periodically from process, which can not be locked,
 *             i.e. from independent high priority thread / timer etc.
 * */
error_e flush_report_buf(void)
{
    int size = sizeof(txHandle.Report.buf) / sizeof(txHandle.Report.buf[0]);
    int chunk;
    error_e ret = _NO_ERR;
    uint32_t tmr;

#ifndef BT_UART_ENABLE
    if (!get_uartEn()
#ifdef USB_ENABLE
        && (UsbGetState() != USB_CONFIGURED)
#endif
    )
        return _ERR_Usb_Tx;
#endif

    QHAL_LOCK(&txHandle); //"return HAL_BUSY;" if locked

    int head = txHandle.Report.head;
    int tail = txHandle.Report.tail;

    int len = CIRC_CNT(head, tail, size);

#ifdef USB_ENABLE
    int old_tail = txHandle.Report.tail;
#endif

    Timer.start(&tmr);

    if (len > 0)
    {
        do
        {
            if (Timer.check(tmr, USB_UART_TX_TIMEOUT_MS))
            {
                break; // max timeout for any output on the 115200bps rate (currently ~1400ms if over the UART)
            }

#ifdef LATER
            /* check the UART status - ready to TX */
            if ((huart3.gState & 0x01) && (get_uartEn() == 1))
            {
                continue; /**< UART is slow and busy, but it does not care of sudden disconnection like USB : wait to complete */
            }
#endif
#ifdef USB_ENABLE
            /* check USB status - ready to TX */
            if ((UsbGetState() == USB_CONFIGURED) && !Usb.isTxBufferEmpty())
            {
                continue; /**< USB did not send the buffer: no connection to the terminal */
            }
#endif

            /* copy MAX allowed length from circular buffer to linear buffer */
            chunk = MIN(sizeof(ubuf), len);

            for (int i = 0; i < chunk; i++)
            {
                ubuf[i] = txHandle.Report.buf[tail];
                tail = (tail + 1) & (size - 1);
            }

            len -= chunk;

            txHandle.Report.tail = tail;

            if (get_uartEn())
            {
                /* setup UART DMA transfer */
                // if (HAL_UART_Transmit_DMA(&huart3, ubuf, chunk) != HAL_OK)
                if (!deca_uart_transmit(ubuf, chunk))
                {
                    error_handler(0, _ERR_UART_TX); /**< indicate UART transmit error */
                    ret = _ERR_UART_TX;
                }
            }
            else
            {
#ifdef USB_ENABLE
                /* setup USB IT transfer */
                // if (CDC_Transmit_FS(ubuf, chunk) != USBD_OK)
                if (!Usb.transmit(ubuf, chunk))
                {
                    error_handler(0, _ERR_Usb_Tx); /**< indicate USB transmit error */
                    txHandle.Report.tail = old_tail;
                    ret = _ERR_Usb_Tx;
                }
                else
                {
                    old_tail = tail;
                }
#endif
#ifdef BT_UART_ENABLE
                bt_uart_transmit(ubuf, chunk);
#endif
            }
        } while (len > 0 && (AppGet()->app_mode & APP_BLOCK_FLUSH));
    }
    QHAL_UNLOCK(&txHandle);
    return ret;
}

// END OF Report section
