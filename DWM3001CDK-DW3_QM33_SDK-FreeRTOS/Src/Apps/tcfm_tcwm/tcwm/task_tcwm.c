/*
 * @file      task_tcwm.c
 *
 * @brief     task for continuous wave test
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

#include "task_tcwm.h"

#include "usb_uart_rx.h"
#include "tcwm.h"
#include "deca_error.h"
#include "deca_dbg.h"
#include "HAL_error.h"
#include "HAL_uwb.h"
#include "task_signal.h"
#include "int_priority.h"
#include "cmd.h"
#ifdef CONFIG_PEG_UWB
#include "uwb_driver_api.h"
#else
#include "deca_device_api.h"
#endif
#include "create_tcwm_task.h"

#define TCWM_TASK_DUMMY_TMR_MS    (1000)

static task_signal_t tcwmTask;

#define TCWM_TASK_STACK_SIZE_BYTES 512

//-----------------------------------------------------------------------------

/* @fn        TcwmTask
 * @brief     this starts the Continuous Wave test functionality.
 *
 *            Note: Previous tasks which can call shared resources must be killed.
 *
 * */
void TcwmTask(void const *argument)
{
    tcwmTask.Exit = 0;

    diag_printf("TCWMTask start\r\n");

    while (tcwmTask.Exit == 0)
    {
        osEvent evt = osSignalWait(tcwmTask.SignalMask, osWaitForever);
        if (evt.value.signals & STOP_TASK)
        {
            break;
        }
        tcwm_process_run();
    }
    tcwmTask.Exit = 2;
    while (tcwmTask.Exit == 2)
    {
        osDelay(1);
    }
}


/* @brief
 *        Kill all tasks and timers related to TcwmTask if any
 *
 *        DW1000's RX and IRQ shall be switched off before task termination,
 *        that IRQ will not produce unexpected Signal
 *
 * */
void tcwm_terminate(void)
{
#ifdef CONFIG_PEG_UWB
    // This is need to stop the continuous wave mode for QM35
    dwt_disablecontinuouswavemode();
#endif
    /*   need to switch off DW chip's RX and IRQ before killing tasks */
    hal_uwb.disableIRQ();
    hal_uwb.reset();
    hal_uwb.deinit_callback(); // hal_deinit_callback(); //DW_IRQ is disabled: safe to cancel all user call-backs

    tcwm_process_terminate();

    terminate_task(&tcwmTask);

    hal_uwb.sleep_enter();
}


/* @fn         tcwm_helper
 * @brief      this is a service function which starts the
 *             Continuous Wave Test functionality
 *             Note: Previous tasks which can access shared resources must be killed.
 *
 * */
void tcwm_helper(void const *arg)
{
    (void)(arg);

    hal_uwb.disable_irq_and_reset(1);

    if (hal_uwb.uwbs != NULL)
    {
        hal_uwb.uwbs->spi->fast_rate(hal_uwb.uwbs->spi->handler);
    }

    error_e ret = create_tcwm_task(TcwmTask, &tcwmTask, (uint16_t)TCWM_TASK_STACK_SIZE_BYTES, (void *)arg);

    if (ret != _NO_ERR)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }


    tcwm_process_init();
}

const app_definition_t helpers_tcwm_node[] __attribute__((section(".known_apps"))) = 
{
    {"TCWM",  mAPP, tcwm_helper,  tcwm_terminate, waitForCommand, command_parser,NULL}
};
