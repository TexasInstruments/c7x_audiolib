// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <stdint.h>

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{

   // clang-format off

   // Setup input and output buffers for single- and double-precision datatypes
   float pIn[] = {-3.0603208541870117, 3.5935628414154053, 2.801513433456421, -0.6980822682380676,
      0.12587422132492065, -8.648187637329102, 0.42226290702819824, -0.8549343943595886,
      -8.268763542175293, -4.231703758239746, -8.94597339630127, 0.7231388092041016,
      9.496573448181152, 6.549774169921875, 3.114142656326294, 4.5740766525268555,
   -9.670684814453125, 2.817779064178467, 4.066615581512451, -6.309268474578857,
      0.6756653189659119, 6.6836934089660645, 5.6005659103393555, -1.6245629787445068,
      2.329049587249756, 2.781999111175537, 3.663892984390259, 2.9200401306152344,
      3.4790844917297363, -6.111962795257568, 2.1385388374328613, -2.1977176666259766,
   8.619332313537598, 0.2276780903339386, -1.6925296783447266, -3.9961934089660645,
      -0.7178579568862915, -7.795764923095703, -8.125761985778809, 3.2559940814971924,
      1.8920129537582397, -3.290354013442993, -2.874565601348877, -2.4495580196380615,
      3.8208234310150146, -7.952194690704346, 1.6919658184051514, -5.3173346519470215,
   -9.94766902923584, -2.492143392562866, 5.680093288421631, -5.298727512359619,
      7.638338088989258, 9.194639205932617, -5.751791954040527, -5.094882965087891,
      -6.492748260498047, 2.4435176849365234, 6.9688615798950195, -2.556135892868042,
      3.254002332687378, 3.7920913696289062, 0.6481939554214478, -3.077218532562256,
   -3.593235492706299, 5.7729973793029785, -2.4166975021362305, -0.5896821022033691,
      -2.4583137035369873, 8.20766830444336, -5.605325222015381, -3.4045915603637695,
      -3.0851528644561768, 1.2352076768875122, -0.6445872187614441, 6.234692096710205,
      -1.737377405166626, 9.234813690185547, 2.3002982139587402, -5.562257289886475,
   8.106548309326172, -5.959496974945068, -5.459980010986328, 6.58470344543457,
      -4.107727527618408, 8.01581859588623, -8.96323013305664, 4.761518478393555,
      5.30494499206543, -8.035195350646973, 1.5002789497375488, -8.937873840332031,
      -7.198543548583984, -5.1163763999938965, -0.2703162133693695, -2.1323366165161133,
   -8.990683555603027, -4.231927394866943, -5.933426856994629, 1.965046763420105,
      -8.164145469665527, 0.47626370191574097, -9.853971481323242, 0.9554539322853088,
      1.3172528743743896, 2.862342596054077, -3.2948060035705566, -6.173038482666016,
      8.289447784423828, -1.8249136209487915, -4.076364040374756, 8.93314266204834};
   float pOutChannels[] = {0,9};
   float pOut[] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
      0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
   float pOutScratch[] = {0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS          status;
   AUDIOLIB_router_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_router_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = calloc(1, handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn00Temp, bufParamsOutTemp;
   AUDIOLIB_bufParams1D_t bufParamsIn01Temp;

   // fill in input and output buffer parameters
   bufParamsIn00Temp.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn00Temp.dim_x     = 16;
   bufParamsIn00Temp.dim_y     = 7;
   bufParamsIn00Temp.stride_y  = 16 * sizeof(float);

   bufParamsIn01Temp.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn01Temp.dim_x     = 2;

   bufParamsOutTemp.data_type = AUDIOLIB_FLOAT32;
   bufParamsOutTemp.dim_x     = 16;
   bufParamsOutTemp.dim_y     = 2;
   bufParamsOutTemp.stride_y  = 16 * sizeof(float);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.isInterleave = 0;
   kerInitArgs.numOutputs   = 2;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_router_init_checkParams(handle, &bufParamsIn00Temp, &bufParamsIn01Temp, &bufParamsOutTemp,
                                                &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_router_init(handle, &bufParamsIn00Temp, &bufParamsIn01Temp, &bufParamsOutTemp, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_router_set(handle, pOutChannels);
   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_router_exec_checkParams(handle, pIn, pOutChannels, pOut, pOutScratch);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_router_exec(handle, pIn, pOut, pOutScratch);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < 2; i++) {
      printf("\n\n");
      for (size_t j = 0; j < 16; j++) {
         printf("%10g, ", pOut[i * 16 + j]);
      }
   }

   printf("\n\n");
   free(handle);

   return 0;
}
