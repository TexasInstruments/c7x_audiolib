// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "ti_board_open_close.h"

int32_t Board_driversOpen(void)
{
   int32_t status = SystemP_SUCCESS;
   if (0 == Armv8_getCoreId()) {
   }
   return status;
}

void Board_driversClose(void)
{
   if (0 == Armv8_getCoreId()) {
   }
}
