/**
 * @file      task_uci.c
 *
 * @brief     uci example task
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

#include <assert.h>
#include <stdlib.h>

#include "cmd.h"
#include "cmd_fn.h"
#include "controlTask.h"
#include "create_uci_task.h"
#include "critical_section.h"
#include "reporter.h"
#include "task_uci.h"
#include "uci_backend/default_coordinator.h"
#include "uci_backend/uci_backend_coordinator.h"
#include "uci_backend/uci_backend_mac.h"
#include "uci_backend/uci_backend_pctt.h"
#include "uci_parser.h"
#include "uci_transport.h"
#include "uwbmac_helper.h"
#include "cmd.h"
#include "HAL_uwb.h"
#include "rtls_version.h"
#include "common_fira.h"

#ifdef ESE_ENABLE
#include "uci_se_backend.h"
#endif
#ifdef LLD_ENABLE
#include "uci_backend_llp2mp.h"
#endif
#ifdef USE_UCI_HSSPI
#include "HAL_hsspi.h"
#endif
#ifdef USE_UCI_UART_1
#include "HAL_uci_uart.h"
#endif

#define UCI_TASK_STACK_SIZE_BYTES 4096
#define UCI_RX_BUF_SIZE           512

extern void pdoaupdate_lut(void);

static task_signal_t uciTask;
extern const app_definition_t helpers_uci_node[];

static uint8_t uci_rx_buf[UCI_RX_BUF_SIZE];
static uint16_t uci_rx_len;

static const struct uci_transport_ops uci_usb_ops = {
    .attach            = uci_tp_attach,
    .detach            = uci_tp_detach,
    .packet_send_ready = uci_tp_usb_packet_send_ready,
};

static struct uci_tp tr = {
    .read_buf        = (char *)&uci_rx_buf[0],
    .p_read_buf_size = &uci_rx_len,
    .tr              = {.ops = NULL},
    .uci             = NULL,
    .uci_if          = UCI_NONE,
};

#ifdef USE_UCI_HSSPI
static const struct uci_transport_ops uci_hsspi_ops = {
    .attach            = uci_tp_attach,
    .detach            = uci_tp_detach,
    .packet_send_ready = uci_tp_hsspi_packet_send_ready,
};
#endif

#ifdef USE_UCI_UART_1
static const struct uci_transport_ops uci_uart1_ops = {
    .attach            = uci_tp_attach,
    .detach            = uci_tp_detach,
    .packet_send_ready = uci_tp_uart1_packet_send_ready,
};
#endif

static struct uci_blk *simple_alloc(struct uci_allocator *uci, size_t size_hint)
{
    struct uci_blk *ret;

    ret = (struct uci_blk *)malloc(sizeof(*ret) + size_hint);
    if (ret)
    {
        ret->data = (uint8_t *)&ret[1];
        ret->size = UCI_MAX_PACKET_SIZE;
        ret->len  = 0;
    }

    return ret;
}

static void simple_free(struct uci_allocator *uci, struct uci_blk *b)
{
    free(b);
}

static const struct uci_allocator_ops simple_allocator_ops = {
    .alloc = simple_alloc,
    .free  = simple_free,
};

static const struct uci_allocator simple_allocator = {
    .ops = &simple_allocator_ops
};

static struct
{
    struct uwbmac_context *uwbmac_ctx;
    struct uci uci_server;
    struct default_coordinator coord;
    struct uci_backend_fira_context fira_ctx;
    struct uci_backend_mac_context mac_ctx;
    struct uci_backend_pctt_context pctt_ctx;
    struct uci_backend_manager sess_man;
} ctx;

void uci_reset_cb(uint8_t reason, void *user_data)
{
    uci_backend_fira_send_reset_response(&ctx.uci_server, true);
    osDelay(10);
    uci_sys_reset();
    while (1);
}

//-----------------------------------------------------------------------------
/* @fn        UciTask
 * @brief     this starts the UCI functionality.
 *
 *            Note: Previous tasks which can call shared resources must be killed.
 *            This task needs the RAM size of at least uci_t
 *
 * */
void uci_task(void const *argument)
{
    int err;

    uciTask.Exit = 0;
    /* Initialize MCPS */
    enter_critical_section();
    uwbmac_helper_init_mcps();
    leave_critical_section();

    /* Initialize MAC */
    err = uwbmac_init(&ctx.uwbmac_ctx);
    assert(err == UWBMAC_SUCCESS);


    /* Initialize UCI server */
    uwbmac_error ret;
    ret = uci_init(&ctx.uci_server, (struct uci_allocator *)&simple_allocator, false);
    assert(ret == UWBMAC_SUCCESS);

    /* Initialize coordinator and session manager */
    default_coordinator_init(&ctx.coord, ctx.uwbmac_ctx);

    /* Initialize fira/CCC backend */
    uci_backend_manager_init(&ctx.sess_man, &ctx.uci_server, ctx.uwbmac_ctx);

#ifdef UCI_FIRA_BACKEND
    /* Initilize UWB driver for FiRa */
    enter_critical_section();
    uwbmac_helper_init_fira();
    leave_critical_section();

    /* Initialize FiRa backend */
    err = uci_backend_fira_init(&ctx.fira_ctx, &ctx.uci_server, ctx.uwbmac_ctx, &ctx.coord.base, &ctx.sess_man);
    assert(!err);

    if (hal_uwb.is_aoa() == AOA_ENABLED)
    {
        ctx.fira_ctx.antennas->aoa_capability = 1;
    }
    else
    {
        ctx.fira_ctx.antennas->aoa_capability = 0;
    }
#endif

#ifdef UCI_PCTT_BACKEND
    /* Initialize PCTT backend */
    err = uci_backend_pctt_init(&ctx.pctt_ctx, &ctx.uci_server, ctx.uwbmac_ctx, &ctx.coord.base, &ctx.sess_man);
    assert(!err);
#endif

#ifdef LLD_ENABLE
    /* Init LLP2MP backend */
    uci_backend_llp2mp_register(&ctx.uci_server, ctx.uwbmac_ctx);
#endif

#ifdef ESE_ENABLE
    uci_se_backend_register(&ctx.uci_server);
#endif

#ifdef USE_UCI_HSSPI
    uci_rx_len = 0;
    hsspi_read(tr.read_buf, UCI_MAX_PACKET_SIZE);
#endif

#ifdef USE_UCI_UART_1
    memset(uci_rx_buf, 0, sizeof(uci_rx_buf));
    uci_uart_init();
#endif

    uint8_t device_info[12];
    uint32_t device_id = dwt_readdevid();
    uint32_t part_id   = dwt_getpartid();

    device_info[0] = device_id & (0xFF);
    device_info[1] = (device_id >> 8) & (0xFF);
    device_info[2] = (device_id >> 16) & (0xFF);
    device_info[3] = (device_id >> 24) & (0xFF);

    device_info[4] = part_id & (0xFF);
    device_info[5] = (part_id >> 8) & (0xFF);
    device_info[6] = (part_id >> 16) & (0xFF);
    device_info[7] = (part_id >> 24) & (0xFF);

    device_info[8]  = VER_PATCH;
    device_info[9]  = VER_MINOR;
    device_info[10] = VER_MAJOR;

    device_info[11] = hal_uwb.is_aoa();

    uci_backend_manager_set_vendor_data(&ctx.sess_man, device_info, sizeof(device_info));

    uci_backend_fira_set_reset_callback(&ctx.fira_ctx, uci_reset_cb, NULL);

    while (uciTask.Exit == 0)
    {
        osEvent evt = osSignalWait(uciTask.SignalMask, osWaitForever);
        if (evt.value.signals & STOP_TASK)
        {
            break;
        }

        uci_task_process();
    }
    uciTask.Exit = 2;
    while (uciTask.Exit == 2)
    {
        osDelay(1);
    };
}

void uci_task_process()
{
    int ret;

    switch (tr.uci_if)
    {
    case (UCI_UART0):
        ret = uci_tp_read(&tr);
        if (ret > 0)
        {
            memset(local_buff, 0, sizeof(local_buff));
            local_buff_length = 0;
        }
        break;
#ifdef USE_UCI_UART_1
    case (UCI_UART1):
        uci_rx_len = uci_uart_read(uci_rx_buf, sizeof(uci_rx_buf));
        ret        = uci_tp_read(&tr);
        break;
#endif
#ifdef USE_UCI_HSSPI
    case (UCI_HSSPI):
        uci_rx_len = hsspi_data_received();
        ret        = uci_tp_read(&tr);
        break;
#endif
    }
}

/* @brief
 * Kill all tasks and timers related to uci if any
 *
 * */
void uci_terminate(void)
{
    terminate_task(&uciTask);

    uci_uninit(&ctx.uci_server);

    uci_process_terminate();

#ifdef UCI_FIRA_BACKEND
    uci_backend_fira_release(&ctx.fira_ctx);
#endif

#ifdef UCI_PCTT_BACKEND
    uci_backend_pctt_release(&ctx.pctt_ctx);
#endif

#ifdef ESE_ENABLE
    uci_se_backend_unregister();
#endif

    uwbmac_exit(ctx.uwbmac_ctx);
    uwbmac_helper_deinit();

    hal_uwb.sleep_enter();
}

error_e uci_process_init(void)
{
    error_e ret = _NO_ERR;

    local_buff_length = 0;
    memset(local_buff, 0, sizeof(local_buff));

    tr.uci_if = UCI_NONE;

    return (ret);
}

/*
 * @brief     stop function implements the stop functionality if any suitable for current process
 *             which will be executed on reception of Stop command
 * */
void uci_process_terminate(void)
{
    tr.uci_if = UCI_NONE;

#ifdef CLI_BUILD
    const char return_val[] = "Ok\r\n";

    tr.uci_if = UCI_NONE;

    enter_critical_section();

//    port_stop_all_UWB();
#ifdef USE_UCI_UART_1
    uci_uart_disable();
#endif
    reporter_instance.print((char *)return_val, sizeof(return_val) - 1);

    leave_critical_section();
#endif
}

/* @fn         uci_helper
 * @brief      this is a service function which starts the
 *             uci test functionality
 * */
void uci_helper(void const *arg)
{
    error_e tmp;

    // hal_uwb.disable_irq_and_reset(0);

    set_local_pavrg_size();
    pdoaupdate_lut();

    uciTask.task_stack = NULL;
    error_e ret        = create_uci_task(uci_task, &uciTask, (uint16_t)UCI_TASK_STACK_SIZE_BYTES, arg);

    if (ret != _NO_ERR)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }

    tmp = uci_process_init();

    if (tmp != _NO_ERR)
    {
        error_handler(1, tmp);
    }
}

void uci_task_notify(usb_data_e res, char *text)
{
#ifdef CLI_BUILD
    if (res == DATA_STOP)
    {
        command_stop_received();
    }
    else if (res == DATA_SAVE)
    {
        AppSetDefaultEvent(&helpers_uci_node[0]);
        uci_tp_flush(&tr);
    }
    else if (res == DATA_READY)
#else
    if (res == DATA_READY)
#endif
    {
        uci_interface_select(UCI_UART0);
        if (tr.uci_if == UCI_UART0)
        {
            osSignalSet(uciTask.Handle, UCI_DATA);
        }
    }
}

#ifdef USE_UCI_UART_1
void uci_uart_notify(void)
{
    uci_interface_select(UCI_UART1);
    if (tr.uci_if == UCI_UART1)
    {
        osSignalSet(uciTask.Handle, UCI_DATA);
    }
}
#endif

#ifdef USE_UCI_HSSPI
void uci_hsspi_notify()
{
    uci_interface_select(UCI_HSSPI);
    if (tr.uci_if == UCI_HSSPI)
    {
        osSignalSet(uciTask.Handle, UCI_DATA);
    }
}
#endif

void uci_interface_select(uint8_t interface)
{
    if (tr.uci_if == UCI_NONE)
    {
        switch (interface)
        {
        case UCI_UART0:
            tr.uci_if          = UCI_UART0;
            tr.read_buf        = (char *)&local_buff[0];
            tr.p_read_buf_size = &local_buff_length;
            tr.tr.ops          = &uci_usb_ops;
            break;
#ifdef USE_UCI_UART_1
        case UCI_UART1:
            tr.uci_if          = UCI_UART1;
            tr.read_buf        = (char *)&uci_rx_buf[0];
            tr.p_read_buf_size = &uci_rx_len;
            tr.tr.ops          = &uci_uart1_ops;
            break;
#endif
#ifdef USE_UCI_HSSPI
        case UCI_HSSPI:
            tr.uci_if          = UCI_HSSPI;
            tr.read_buf        = (char *)&uci_rx_buf[0];
            tr.p_read_buf_size = &uci_rx_len;
            tr.tr.ops          = &uci_hsspi_ops;
            break;
#endif
        }
        uci_transport_attach(&ctx.uci_server, &tr.tr);
    }
}

const app_definition_t helpers_uci_node[] __attribute__((section(".known_apps"))) = {
#ifdef USE_UCI_UART_1
    {"UCI", mAPP | APP_SAVEABLE, uci_helper, uci_terminate, waitForCommand, command_parser}
#else
    {"UCI", mAPP | APP_SAVEABLE, uci_helper, uci_terminate, cir_to_ser_buf, uci_task_notify, NULL}
#endif
};
