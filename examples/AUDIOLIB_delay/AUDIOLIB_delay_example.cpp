// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "audiolib.h"
#include <stdint.h>

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{

   // Setup input and output buffers for single- and double-precision datatypes
   float in[] = {0.0075269,  0.42490765, 0.5737673,  0.2208442,  0.09775102, 0.24138574, 0.04012135, 0.74615402,
                 0.08575411, 0.02825585, 0.0394516,  0.53011919, 0.8855483,  0.64490047, 0.43757865, 0.9008144,
                 0.88742952, 0.5273014,  0.15618296, 0.2486165,  0.73751811, 0.80145564, 0.00925736, 0.61880944,
                 0.01848634, 0.78157248, 0.29912587, 0.34630224, 0.28394529, 0.16470226, 0.03790734, 0.05151005,
                 0.93717938, 0.93691852, 0.85173587, 0.99901312, 0.72999805, 0.07310897, 0.68994449, 0.80322511,
                 0.59070248, 0.52609875, 0.8837833,  0.43461102, 0.29789329, 0.8795983,  0.08831519, 0.25615793,
                 0.57545443, 0.76529679, 0.7559087,  0.60065098, 0.93648813, 0.9509784,  0.33040872, 0.07305676,
                 0.79920768, 0.03319901, 0.90727184, 0.59441656, 0.42762858, 0.92020711, 0.80031864, 0.01848634};

   float out[] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                  0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

   float scratch[] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                      0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

   float delay[] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                    0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

   // handles and struct for call to kernel
   AUDIOLIB_STATUS         status;
   AUDIOLIB_delay_InitArgs kerInitArgs;
   int32_t                 handleSize = AUDIOLIB_delay_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle   handle     = malloc(handleSize);

   AUDIOLIB_bufParams1D_t bufParamsIn, bufParamsOut, bufParamsDelay;

   // fill in input and output buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 32;
   bufParamsIn.dim_y     = 2;
   bufParamsIn.stride_y  = 32 * AUDIOLIB_sizeof(bufParamsIn.data_type);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 16;
   bufParamsOut.dim_y     = 2;
   bufParamsOut.stride_y  = 16 * AUDIOLIB_sizeof(bufParamsOut.data_type);

   bufParamsDelay.data_type = AUDIOLIB_FLOAT32;
   bufParamsDelay.dim_x     = 48;
   bufParamsDelay.dim_y     = 2;
   bufParamsDelay.stride_y  = 48 * AUDIOLIB_sizeof(bufParamsDelay.data_type);

   kerInitArgs.funcStyle  = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.mode       = 0;
   kerInitArgs.interleave = 0;
   kerInitArgs.maxDelay   = 16;
   kerInitArgs.delaySize  = 8;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   //    status = AUDIOLIB_delay_init_checkParams(handle, &bufParamsIn, &bufParamsDelay, &bufParamsOut,
   //    &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_delay_init(handle, &bufParamsIn, &bufParamsDelay, &bufParamsOut, &kerInitArgs);

   // exec checkparams

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_delay_exec(handle, in, delay, out, scratch);

   // print results
   for (size_t c = 0; c < size; c++) {
      printf("%10g + %10g = %10g\n", in[c], delay[c], out[c]);
   }

   return 0;
}
