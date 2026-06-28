// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "ti_drivers_config.h"
#include <drivers/pinmux.h>

static Pinmux_PerCfg_t gPinMuxMainDomainCfg[] = {

    /* USART0 pin config */
    /* UART0_RXD -> UART0_RXD (E14) */
    {PIN_UART0_RXD, (PIN_MODE(0) | PIN_INPUT_ENABLE | PIN_PULL_DISABLE)},
    /* UART0_TXD -> UART0_TXD (D15) */
    {PIN_UART0_TXD, (PIN_MODE(0) | PIN_PULL_DISABLE)},

    {PINMUX_END, 0U}};

static Pinmux_PerCfg_t gPinMuxMcuDomainCfg[] = {

    {PINMUX_END, 0U}};

/*
 * Pinmux
 */
void Pinmux_init(void)
{
   Pinmux_config(gPinMuxMainDomainCfg, PINMUX_DOMAIN_ID_MAIN);
   Pinmux_config(gPinMuxMcuDomainCfg, PINMUX_DOMAIN_ID_MCU);
}
