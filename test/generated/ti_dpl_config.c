// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "ti_dpl_config.h"
#include "ti_drivers_config.h"
#include <drivers/soc.h>
#include <kernel/dpl/AddrTranslateP.h>
#include <stdio.h>

#include <kernel/nortos/dpl/a53/SpinlockP_armv8.h>
uint32_t gSwSpinLockBuff[NO_OF_SW_SPIN_LOCKS] __attribute__((aligned(128), section(".bss.amp_shared_mem")));

/* ----------- DebugP ----------- */
void putchar_(char character)
{
   /* Output to CCS console */
   putchar(character);
   /* Output to UART console */
   DebugP_uartLogWriterPutChar(character);
}

/* ----------- MmuP_armv8 ----------- */
#define CONFIG_MMU_NUM_REGIONS (2u)

MmuP_Config gMmuConfig = {
    .numRegions = CONFIG_MMU_NUM_REGIONS,
    .enableMmu  = 1,
};

MmuP_RegionConfig gMmuRegionConfig[CONFIG_MMU_NUM_REGIONS] = {
    {.vaddr = 0x0u,
     .paddr = 0x0u,
     .size  = 0x80000000u,
     .attr  = {.accessPerm  = MMUP_ACCESS_PERM_PRIV_RW_USER_NONE,
               .privExecute = 1,
               .userExecute = 0,
               .shareable   = MMUP_SHARABLE_OUTER,
               .attrIndx    = MMUP_ATTRINDX_MAIR0,
               .global      = 1}},
    {.vaddr = 0x80000000u,
     .paddr = 0x80000000u,
     .size  = 0x80000000u,
     .attr  = {.accessPerm  = MMUP_ACCESS_PERM_PRIV_RW_USER_NONE,
               .privExecute = 1,
               .userExecute = 0,
               .shareable   = MMUP_SHARABLE_OUTER,
               .attrIndx    = MMUP_ATTRINDX_MAIR7,
               .global      = 1}},
};

/* ----------- ClockP ----------- */
#define TIMER6_CLOCK_SRC_MUX_ADDR (0x1081C8u)
#define TIMER6_CLOCK_SRC_MCU_HFOSC0 (0x0u)
#define TIMER6_BASE_ADDR (0x2460000u)

ClockP_Config gClockConfig = {
    .timerBaseAddr       = TIMER6_BASE_ADDR,
    .timerHwiIntNum      = 158,
    .timerInputClkHz     = 25000000,
    .timerInputPreScaler = 1,
    .usecPerTick         = 1000,
};

/* This function is called by __system_start */
void __mmu_init()
{
   MmuP_init();
   CacheP_enable(CacheP_TYPE_ALL);
}

void Dpl_init(void)
{

   /* initialize Hwi but keep interrupts disabled */
   HwiP_init();

   /* init debug log zones early */
   if (0 == Armv8_getCoreId()) {
      /* Debug log init */
      DebugP_logZoneEnable(DebugP_LOG_ZONE_ERROR);
      DebugP_logZoneEnable(DebugP_LOG_ZONE_WARN);
      /* UART console to use for reading input */
      DebugP_uartSetDrvIndex(CONFIG_UART0);
   }

   /* set timer clock source */
   SOC_controlModuleUnlockMMR(SOC_DOMAIN_ID_MAIN, 2);
   *(volatile uint32_t*) (TIMER6_CLOCK_SRC_MUX_ADDR) = TIMER6_CLOCK_SRC_MCU_HFOSC0;
   SOC_controlModuleLockMMR(SOC_DOMAIN_ID_MAIN, 2);
   if (0 == Armv8_getCoreId()) {
      /* initialize Clock */
      ClockP_init();
   }

   /* Enable interrupt handling */
   HwiP_enable();
}

void Dpl_deinit(void)
{
   /* de-initialize Clock */
   ClockP_deinit();
   /* Disable interrupt handling */
   HwiP_disable();
}
