/**
 * @file      controlTask.c
 *
 * @brief     Control task for USB/UART
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

//#include <cmsis_os.h>
#include <stdlib.h>

#include "app.h"
#include "circular_buffers.h"
#include "cmd.h"
#include "cmd_fn.h"
#include "create_control_task.h"
#include "int_priority.h"
#include "task_signal.h"
#include "usb_uart_rx.h"
#include "usb_uart_tx.h"

#define CONTROL_TASK_STACK_SIZE_BYTES 2048

task_signal_t ctrlTask;

#define COM_RX_BUF_SIZE UART_RX_BUF_SIZE /*USB_RX_BUF_SIZE*/ /**< Communication RX buffer size */
uint16_t local_buff_length;                                  /**< from usb_uart_rx parser to application */
uint8_t local_buff[COM_RX_BUF_SIZE];                         /**< for RX from USB/USART */

/**
 * @brief     this is a Command Control and Data task.
 *             this task is activated on the startup
 *             there 2 sources of control data: Uart and Usb.
 *
 * */
static void CtrlTask(void const *arg)
{
    usb_data_e res;

    while (1)
    {
        osEvent evt = osSignalWait(ctrlTask.SignalMask, osWaitForever); /* signal from USB/UART that some data has been received */

#ifdef CLI_BUILD
        if (evt.value.signals & CTRL_STOP_APP)
        {
            osDelay(500);//time for flushing, TODO: create a function for testing flushing is over
            command_stop_received();
        }
#endif
        if (evt.value.signals & CTRL_DATA_RECEIVED)
        {
            enter_critical_section();

            /* mutex if usb2spiTask using the app.local_buf*/
            res = usb_uart_rx(); /*< processes usb/uart input :
                                     copy the input to the app.local_buff[ local_buff_length ]
                                     for future processing */
            leave_critical_section();

            AppGet()->command_parser(res, (char *)local_buff);
        }
    }
}

/* definition and creation of ControlTask */
/* Note. The ControlTask awaits an input on a USB and/or UART interfaces. */
void ControlTaskInit(void)
{
    error_e ret = create_control_task(CtrlTask, &ctrlTask, (uint16_t)CONTROL_TASK_STACK_SIZE_BYTES);

    if (ret != _NO_ERR)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }
}

void NotifyControlTask(void)
{
    if (ctrlTask.Handle != NULL) // RTOS : ctrlTask could be not started yet
    {
        osSignalSet(ctrlTask.Handle, CTRL_DATA_RECEIVED); // signal to the ctrl thread : USB data ready
    }
}

void NotifyControlTaskStopApp(void)
{
    if (ctrlTask.Handle != NULL) // RTOS : ctrlTask could be not started yet
    {
        osSignalSet(ctrlTask.Handle, CTRL_STOP_APP); // signal to the ctrl thread : USB data ready
    }
}