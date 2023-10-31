/**
 * @file      task_listener2.c
 *
 * @brief     Listener task functionalities
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
 */

#include <math.h>
#include "cmsis_os.h"
#include "critical_section.h"

#include "app.h"
#include "deca_dbg.h"
#include "util.h"
#include "usb_uart_tx.h"
#include "listener2.h"
#include "task_signal.h"
#include "HAL_error.h"
#include "HAL_uwb.h"
#include "int_priority.h"
#include "circular_buffers.h"
#include "usb_uart_tx.h"
#include "cmd_fn.h"
#include "minmax.h"
#include "flushTask.h"
#include"cmd.h"
#include "reporter.h"
#include "create_listener_task.h"

static task_signal_t listenerTask;

extern const struct command_s known_commands_listener;

#define LISTENER_TASK_STACK_SIZE_BYTES 2048

//-----------------------------------------------------------------------------

/*
 * @brief function to report to PC the Listener data received
 *
 * 'JSxxxx{"LSTN":[RxBytes_hex,..,],"TS":"0xTimeStamp32_Hex","O":Offset_dec}'
 *
 * This fn() uses pseudo-JSON format, it appends "+" to the end of
 *
 * */
#define MAX_PRINT_FAST_LISTENER (6)
#define MAX_STR_SIZE            255
error_e send_to_pc_listener_info(uint8_t *data, uint8_t size, uint8_t *ts, int16_t cfo, int mode, int rsl100, int fsl100)
{
    error_e ret = _ERR_Cannot_Alloc_Memory;

    uint32_t cnt, flag_plus = 0;
    uint16_t hlen;
    int      cfo_pphm;
    char     *str;

    if (mode == 0)
    { // speed is a priority
        if (size > MAX_PRINT_FAST_LISTENER)
        {
            flag_plus = 1;
            size = MAX_PRINT_FAST_LISTENER;
        }

        str = CMD_MALLOC(MAX_STR_SIZE);
    }
    else
    {
        str = CMD_MALLOC(MAX_STR_SIZE + MAX_STR_SIZE);
    }

    size = MIN((sizeof(str) - 21) / 3, size); // 21 is an overhead

    if (str)
    {

        cfo_pphm = (int)((float)cfo * (CLOCK_OFFSET_PPM_TO_RATIO * 1e6 * 100));


        hlen = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object
        sprintf(&str[strlen(str)], "{\"LSTN\":[");


        // Loop over the received data
        for (cnt = 0; cnt < size; cnt++)
        {
            sprintf(&str[strlen(str)], "%02X,", data[cnt]); // Add the byte and the delimiter - "XX,"
        }

        if (flag_plus)
        {
            sprintf(&str[strlen(str)], "+,");
        }

        sprintf(&str[strlen(str) - 1], "],\"TS4ns\":\"0x%02X%02X%02X%02X\",", ts[4], ts[3], ts[2], ts[1]);
        sprintf(&str[strlen(str)], "\"O\":%d", cfo_pphm);

        sprintf(&str[strlen(str)], ",\"rsl\":%d.%02d,\"fsl\":%d.%02d", rsl100 / 100, (rsl100 * -1) % 100, fsl100 / 100, (fsl100 * -1) % 100);

        sprintf(&str[strlen(str)], "%s", "}\r\n");
        sprintf(&str[2], "%04X", strlen(str) - hlen);   // add formatted 4X of length, this will erase first '{'
        str[hlen] = '{';                                // restore the start bracket
        ret = copy_tx_msg((uint8_t *)str, strlen(str)); // do not notify flush task, only copy the message

        CMD_FREE(str);
    }

    return (ret);
}

//-----------------------------------------------------------------------------


/* @brief DW3000 RX : Listener RTOS implementation
 *          this is a high-priority task, which will be executed immediately
 *          on reception of waiting Signal. Any task with lower priority will be interrupted.
 *          No other tasks in the system should have higher priority.
 * */
static void ListenerTask(void const *arg)
{
    (void)arg;
    int head, tail, size;
    listener_info_t *pListenerInfo;
    while (!(pListenerInfo = getListenerInfoPtr()))
    {
        osDelay(5);
    }

    size = sizeof(pListenerInfo->rxPcktBuf.buf) / sizeof(pListenerInfo->rxPcktBuf.buf[0]);

    listenerTask.Exit = 0;

    enter_critical_section();
    dwt_rxenable(DWT_START_RX_IMMEDIATE); // Start reception on the Listener
    leave_critical_section();

    while (listenerTask.Exit == 0)
    {
        /* ISR is delivering RxPckt via circ_buf & Signal.
         * This is the fastest method.
         * */
        osEvent evt = osSignalWait(listenerTask.SignalMask, osWaitForever);
        if (evt.value.signals & STOP_TASK)
        {
            break;
        }

        enter_critical_section();
        head = pListenerInfo->rxPcktBuf.head;
        tail = pListenerInfo->rxPcktBuf.tail;
        leave_critical_section();

        if (CIRC_CNT(head, tail, size) > 0)
        {
            rx_listener_pckt_t *pRx_listener_Pckt = &pListenerInfo->rxPcktBuf.buf[tail];

            send_to_pc_listener_info(pRx_listener_Pckt->msg.data,
                                     pRx_listener_Pckt->rxDataLen,
                                     pRx_listener_Pckt->timeStamp,
                                     pRx_listener_Pckt->clock_offset,
                                     listener_get_mode(),
                                     pRx_listener_Pckt->rsl100,
                                     pRx_listener_Pckt->fsl100);


            enter_critical_section();
            tail = (tail + 1) & (size - 1);
            pListenerInfo->rxPcktBuf.tail = tail;
            leave_critical_section();

            NotifyFlushTask();
        }

        osThreadYield();
    };
    listenerTask.Exit = 2;
    while (listenerTask.Exit == 2)
    {
        osDelay(1);
    }
}

void listener_task_notify(void)
{
    if (listenerTask.Handle) // RTOS : listenerTask can be not started yet
    {
        // Sends the Signal to the application level via OS kernel.
        // This will add a small delay of few us, but
        // this method make sense from a program structure point of view.
        if (osSignalSet(listenerTask.Handle, LISTENER_DATA) == 0x80000000)
        {
            error_handler(1, _ERR_Signal_Bad);
        }
    }
}

bool listener_task_started(void)
{
    return listenerTask.Handle != NULL;
}

//-----------------------------------------------------------------------------

/* @brief Setup Listener task, this task will send to received data to UART.
 * Only setup, do not start.
 * */
static void listener_setup_tasks(void)
{
    /* listenerTask is receive the signal from
     * passing signal from RX IRQ to an actual two-way ranging algorithm.
     * It awaiting of an Rx Signal from RX IRQ ISR and decides what to do next in TWR exchange process
     * */
    listenerTask.task_stack = NULL;
    error_e ret = create_listener_task((void *)ListenerTask, &listenerTask, (uint16_t)LISTENER_TASK_STACK_SIZE_BYTES);

    if (ret != _NO_ERR)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }
}

/* @brief Terminate all tasks and timers related to Node functionality, if any
 *        DW3000's RX and IRQ shall be switched off before task termination,
 *        that IRQ will not produce unexpected Signal
 * */
void listener_terminate(void)
{
    /*   need to switch off DW chip's RX and IRQ before killing tasks */
    hal_uwb.disableIRQ();
    hal_uwb.reset();

    terminate_task(&listenerTask);

    listener_process_terminate();

    hal_uwb.sleep_enter();
}


/* @fn         listener_helper
 * @brief      this is a service function which starts the
 *             TWR Node functionality
 *             Note: the previous instance of TWR shall be killed
 *             with node_terminate_tasks();
 *
 *             Note: the node_process_init() will allocate the memory of sizeof(node_info_t)
 *                   from the <b>caller's</b> task stack, see _malloc_r() !
 *
 * */
void listener_helper(void const *argument)
{
    error_e tmp;

    hal_uwb.disable_irq_and_reset(1);

    /* "RTOS-independent" part : initialization of two-way ranging process */
    tmp = listener_process_init(); /* allocate ListenerInfo */

    if (tmp != _NO_ERR)
    {
        error_handler(1, tmp);
    }

    listener_setup_tasks(); /**< "RTOS-based" : setup (not start) all necessary tasks for the Node operation. */

    listener_process_start(); /**< IRQ is enabled from MASTER chip and it may receive UWB immediately after this point */
}

const app_definition_t helpers_app_listener[] __attribute__((section(".known_apps"))) = 
{
    {"LISTENER", mAPP, listener_helper,  listener_terminate, waitForCommand, command_parser, &known_commands_listener}
};
