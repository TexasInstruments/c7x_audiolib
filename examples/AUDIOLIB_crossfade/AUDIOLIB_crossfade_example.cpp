// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{

   // clang-format off

  // Setup input and output buffers for single- and double-precision datatypes
   float pIn0[ ] = {8.768375269168946, -2.122177725806795,  
                       -5.559276500381056, 4.953866389809505,  
                       -9.650125815169782, -6.196634971809694,  
                       -6.219920409550195, -4.2444023516019485,  
                        2.554947242000301, -3.653444935350077,  
                        -1.3589125543960954, 9.30570955993258,  
                        0.4506287864895242, -2.331623545549739,  
                       -4.316918128713447, 2.2144402019478093,  
                       -6.454836498407149, 8.995434312803297,  
                        9.640345081220353, 3.0969734319705093};
   float pIn1[ ] = {5.081505695831783, -1.4776094430247824,
                        -5.770007642516839, 5.632612369737915,  
                         4.8427805198678655, 6.34758980622188,  
                         9.165516794878965, 3.905373842297177,  
                         6.406340472533238, -8.099243409060433,  
                         9.466786222856484, 1.054635835327014,  
                         0.9203609398705037, 9.320457951144235,  
                        -8.504168542150643, 2.446592614841748,  
                         2.519373837270834, -9.465179004027721,  
                        -8.111425636784928, -7.447719563013253};

   float pIn2[ ] = {1.0, 0.984807753012208, 0.9396926207859084, 0.8660254037844387, 0.766044443118978, 0.6427876096865394, 0.5000000000000001, 0.3420201433256688, 0.17364817766693041, 6.123233995736766e-17 
};

   float pIn3[  ] = {0.0, 0.17364817766693033, 0.3420201433256687, 0.49999999999999994, 0.6427876096865393, 0.766044443118978, 0.8660254037844386, 0.9396926207859083, 0.984807753012208, 1.0
};

   float pOut[ ] = {0., 0.,
      0., 0.,
      0., 0.,
      0., 0.,
      0., 0.,
      0., 0.,
      0., 0.,
      0., 0.,
      0., 0.,
      0., 0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS             status;
   AUDIOLIB_crossfade_InitArgs kerInitArgs;

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.isInterleave = 1;

   int32_t               handleSize = AUDIOLIB_crossfade_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsIn1, bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsIn2, bufParamsIn3;

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = 2;
   bufParamsIn0.dim_y     = 10;
   bufParamsIn0.stride_y  = 2 * sizeof(float);

   bufParamsIn1.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn1.dim_x     = 2;
   bufParamsIn1.dim_y     = 10;
   bufParamsIn1.stride_y  = 2 * sizeof(float);

   bufParamsIn2.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn2.dim_x     = 10;

   bufParamsIn3.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn3.dim_x     = 10;

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 2;
   bufParamsOut.dim_y     = 10;
   bufParamsOut.stride_y  = 2 * sizeof(float);

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_crossfade_init_checkParams(handle, &bufParamsIn0, &bufParamsIn1, &bufParamsIn2, &bufParamsIn3,
                                                   &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_crossfade_init(handle, &bufParamsIn0, &bufParamsIn1, &bufParamsIn2, &bufParamsIn3,
                                       &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_crossfade_exec_checkParams(handle, pIn0, pIn1, pIn2, pIn3, pOut);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_crossfade_exec(handle, pIn0, pIn1, pIn2, pIn3, pOut);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < 10; i++) {
      printf("\n\n");
      for (size_t j = 0; j < 2; j++) {
         printf("%10g, ", pOut[i * 2 + j]);
      }
   }

   printf("\n\n");
   free(handle);

   return 0;
}
