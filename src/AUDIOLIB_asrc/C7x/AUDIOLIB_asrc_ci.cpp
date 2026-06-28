// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_asrc_priv.h"
#include "dsplib.h"

#define AUDIOLIB_ASRC_MIN(x, y) (((x) < (y)) ? (x) : (y))

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE2_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE3_PARAM_OFFSET (SE_SE2_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE4_PARAM_OFFSET (SE_SE3_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE4_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA2_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SE_PARAM_SIZE)

/* C7000 minimum circular buffer size in bytes */
#define C7000_MIN_CIRCULAR_BUFFER_SIZE_B (512)

#define ASRC_CHANNEL_UNROLL_FACTOR (2)

/*******************************************************************************
 * INITIALIZATION FUNCTIONS
 ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_non_interleaved(AUDIOLIB_kernelHandle         handle,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                      const AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_init_ci_non_interleaved\n");
#endif

   AUDIOLIB_STATUS         status                  = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs            = (AUDIOLIB_asrc_PrivArgs *) handle;
   uint8_t                *pBlock                  = pKerPrivArgs->bufPblock;
   int32_t                 numChannels             = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 dummyPreCompOutputCount = 1;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se4Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   /**********************************************************************/
   /* Prepare streaming engines to fetch samples                        */
   /**********************************************************************/
   pKerPrivArgs->channelLoopSize = (numChannels + 1) / ASRC_CHANNEL_UNROLL_FACTOR;

   se0Params         = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0   = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
   se0Params.ICNT1   = pKerPrivArgs->channelLoopSize;
   se0Params.DIM1    = pKerPrivArgs->inBufferTotalDimX * (uint32_t) ASRC_CHANNEL_UNROLL_FACTOR;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_2D;
   se0Params.AM0     = __SE_AM_CIRC_CBK0;
   /* Determine state Circular Buffer size.
      CB size = 2^(encoding + 9) bytes. */
   uint32_t cbSizeB       = (uint32_t) C7000_MIN_CIRCULAR_BUFFER_SIZE_B;
   uint32_t encCbSizeB    = 0;
   uint32_t totBlockSizeB = pKerPrivArgs->inBufferTotalStrideY;
   /* The limit of totBlockSizeB is checked in AUDIOLIB_asrc_init_checkParams() through maxSampleCountPerBlock. */
   while (totBlockSizeB > cbSizeB) {
      encCbSizeB++;
      cbSizeB *= 2;
   }
   se0Params.CBK0 = encCbSizeB;

   se1Params               = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0         = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
   se1Params.ICNT1         = pKerPrivArgs->channelLoopSize;
   se1Params.DIM1          = pKerPrivArgs->inBufferTotalDimX * (uint32_t) ASRC_CHANNEL_UNROLL_FACTOR;
   se1Params.ELETYPE       = SE_ELETYPE;
   se1Params.VECLEN        = SE_VECLEN;
   se1Params.AM0           = __SE_AM_CIRC_CBK0;
   se1Params.CBK0          = encCbSizeB;
   se1Params.DECDIM1_WIDTH = pKerPrivArgs->inBufferTotalDimX * (numChannels - 1);
   se1Params.DECDIM1       = __SE_DECDIM_DIM1;
   se1Params.DIMFMT        = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare streaming engines to fetch the filter coefficients         */
   /**********************************************************************/
   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = (int32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS * 2;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_1D;

   /**************************************************************************/
   /* Prepare streaming engines to copy data from rembuf to outbuf           */
   /**************************************************************************/
   se3Params         = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0   = pKerPrivArgs->remBufCount; /* This will be changed in the exec function */
   se3Params.ICNT1   = numChannels;
   se3Params.DIM1    = pKerPrivArgs->bufParamsFilterRembuf.dim_x;
   se3Params.ELETYPE = SE_ELETYPE;
   se3Params.VECLEN  = SE_VECLEN;
   se3Params.DIMFMT  = __SE_DIMFMT_2D;

   /**************************************************************************/
   /* Prepare streaming engines to copy data from outbuf to rembuf           */
   /**************************************************************************/
   se4Params         = __gen_SE_TEMPLATE_v1();
   se4Params.ICNT0   = pKerPrivArgs->remBufCount; /* This will be changed in the exec function */
   se4Params.ICNT1   = numChannels;
   se4Params.DIM1    = pKerPrivArgs->outBufferDimX;
   se4Params.ELETYPE = SE_ELETYPE;
   se4Params.VECLEN  = SE_VECLEN;
   se4Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA template to store output                                */
   /**********************************************************************/
   sa0Params               = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0         = (int32_t) 1;
   sa0Params.ICNT1         = pKerPrivArgs->channelLoopSize * (int32_t) ASRC_CHANNEL_UNROLL_FACTOR;
   sa0Params.ICNT2         = dummyPreCompOutputCount; /* This will be changed in the exec function */
   sa0Params.DIM1          = pKerPrivArgs->outBufferDimX;
   sa0Params.DIM2          = (int32_t) 1;
   sa0Params.VECLEN        = SA_VECLEN;
   sa0Params.DECDIM1_WIDTH = numChannels * pKerPrivArgs->outBufferDimX;
   sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa0Params.DIMFMT        = __SA_DIMFMT_3D;

   /**********************************************************************/
   /* Prepare SA template to copy data from rembuf to outbuf             */
   /**********************************************************************/
   sa1Params        = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0  = pKerPrivArgs->remBufCount; /* This will be changed in the exec function */
   sa1Params.ICNT1  = numChannels;
   sa1Params.DIM1   = pKerPrivArgs->outBufferDimX;
   sa1Params.VECLEN = SA_VECLEN;
   sa1Params.DIMFMT = __SA_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA template to copy data from outbuf to rembuf             */
   /**********************************************************************/
   sa2Params        = __gen_SA_TEMPLATE_v1();
   sa2Params.ICNT0  = pKerPrivArgs->remBufCount; /* This will be changed in the exec function */
   sa2Params.ICNT1  = numChannels;
   sa2Params.DIM1   = pKerPrivArgs->bufParamsFilterRembuf.dim_x;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_2D;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_interleaved(AUDIOLIB_kernelHandle         handle,
                                                  const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                  const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                  const AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_init_ci_interleaved\n");
#endif

   AUDIOLIB_STATUS         status                  = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs            = (AUDIOLIB_asrc_PrivArgs *) handle;
   uint8_t                *pBlock                  = pKerPrivArgs->bufPblock;
   int32_t                 numChannels             = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 dummyPreCompOutputCount = 1;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   /**********************************************************************/
   /* Prepare streaming engines to fetch samples                        */
   /**********************************************************************/
   pKerPrivArgs->channelLoopSize = (numChannels + 1) / ASRC_CHANNEL_UNROLL_FACTOR;

   se0Params         = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0   = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
   se0Params.ICNT1   = pKerPrivArgs->channelLoopSize;
   se0Params.DIM1    = pKerPrivArgs->inBufferTotalDimX * (uint32_t) ASRC_CHANNEL_UNROLL_FACTOR;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.AM0     = __SE_AM_CIRC_CBK0;
   /* Determine state Circular Buffer size.
      CB size = 2^(encoding + 9) bytes. */
   uint32_t cbSizeB       = (uint32_t) C7000_MIN_CIRCULAR_BUFFER_SIZE_B;
   uint32_t encCbSizeB    = 0;
   uint32_t totBlockSizeB = pKerPrivArgs->inBufferTotalStrideY;
   /* The limit of totBlockSizeB is checked in AUDIOLIB_asrc_init_checkParams() through maxSampleCountPerBlock. */
   while (totBlockSizeB > cbSizeB) {
      encCbSizeB++;
      cbSizeB *= 2;
   }
   se0Params.CBK0   = encCbSizeB;
   se0Params.DIMFMT = __SE_DIMFMT_2D;

   se1Params               = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0         = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
   se1Params.ICNT1         = pKerPrivArgs->channelLoopSize;
   se1Params.DIM1          = pKerPrivArgs->inBufferTotalDimX * (uint32_t) ASRC_CHANNEL_UNROLL_FACTOR;
   se1Params.ELETYPE       = SE_ELETYPE;
   se1Params.VECLEN        = SE_VECLEN;
   se1Params.AM0           = __SE_AM_CIRC_CBK0;
   se1Params.CBK0          = encCbSizeB;
   se1Params.DECDIM1_WIDTH = pKerPrivArgs->inBufferTotalDimX * (numChannels - 1);
   se1Params.DECDIM1       = __SE_DECDIM_DIM1;
   se1Params.DIMFMT        = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare streaming engines to fetch the filter coefficients        */
   /**********************************************************************/
   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = (int32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS * 2;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_1D;

   /**********************************************************************/
   /* Prepare SA template to store output                                */
   /**********************************************************************/
   sa0Params               = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0         = (int32_t) 1;
   sa0Params.ICNT1         = pKerPrivArgs->channelLoopSize * (int32_t) ASRC_CHANNEL_UNROLL_FACTOR;
   sa0Params.ICNT2         = dummyPreCompOutputCount; /* This will be changed in the exec function */
   sa0Params.DIM1          = (int32_t) 1;
   sa0Params.DIM2          = pKerPrivArgs->outBufferDimX;
   sa0Params.VECLEN        = SA_VECLEN;
   sa0Params.DECDIM1_WIDTH = numChannels;
   sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa0Params.DIMFMT        = __SA_DIMFMT_3D;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_non_interleaved<float>(AUDIOLIB_kernelHandle         handle,
                                                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                      const AUDIOLIB_asrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_interleaved<float>(AUDIOLIB_kernelHandle         handle,
                                                                  const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                  const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                  const AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/*******************************************************************************
 * EXECUTION FUNCTIONS
 ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_non_interleaved(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pNonInterleavedData,
                                                      void *restrict pFiltCoeffs,
                                                      void *restrict pFilterRembuf,
                                                      void *restrict pOut,
                                                      const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                      AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_exec_ci_non_interleaved\n");
#endif

   AUDIOLIB_STATUS         status             = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs       = (AUDIOLIB_asrc_PrivArgs *) handle;
   uint8_t                *pBlock             = pKerPrivArgs->bufPblock;
   uint32_t               *cirBuffStartIndex  = &pKerPrivArgs->cirBuffStartIndex;
   dataType               *pInLocal           = (dataType *) pIn;
   dataType               *pInLocal2          = pInLocal + pKerPrivArgs->inBufferTotalDimX;
   dataType               *pFilterRembufLocal = (dataType *) pFilterRembuf;
   const dataType         *pFiltCoeffsLocal   = (dataType *) pFiltCoeffs;
   dataType               *pOutLocal          = (dataType *) pOut;
   const uint8_t           numChannels        = pKerPrivArgs->initArgs.numChannels;
   int32_t                 channelLoopSize    = pKerPrivArgs->channelLoopSize;
   uint32_t                inputSampleCount   = pKerInArgs->inputSampleCount;
   int32_t                 preCompOutputCountInt;
   int32_t                 i, k, outSampleIdx;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);

   // output / input time step
   /* maxOutputsPerInputRatio will set the maximum rate ratio change acceptable*/
   /** Accumulator increment value. This variable stores the accumulator increment value, which is calculated as
    * 1/asrcRatio.
    */
   const double rho = 1 / AUDIOLIB_ASRC_MIN(pKerPrivArgs->outputsPerInputRatio, pKerPrivArgs->maxOutputsPerInputRatio);
   /** Accumulator value. This variable stores the accumulator value used in the ARC2 core processing.
    */
   const double tau = rho * (1 - pKerPrivArgs->fracOutputsRemaining);

   // Use rho directly without storing it in pKerPrivArgs->rho
   double preCompOutputCnt = ((double) pKerInArgs->inputSampleCount / rho) + pKerPrivArgs->fracOutputsRemaining;
   /* This condition check has to be done to prevent the algorithm from interpolating the last output sample without an
    * full input sample.
    * Ex:- for 32 kHz -> 48 kHz example,
    *      Assume input sample count = 256
    *      so precomputer output sample count = 256 * 48/32 = 384 samples
    *      But if you try to generate 384 samples in the algorithm, it'll interpolate the last output sample without an
    *      input sample because it has run out of all 256 input samples. This 384th output sample perfectly aligns with
    *      the 257th input sample (if we have one!). So if this perfect alignment happens we need to reduce 1 output
    *      sample. Which means fracOutputsRemaining will be 1;
    */
   if (preCompOutputCnt == (int32_t) preCompOutputCnt) {
      preCompOutputCnt--;
      preCompOutputCountInt              = (int32_t) preCompOutputCnt;
      pKerPrivArgs->fracOutputsRemaining = 1.0;
   }
   else {
      preCompOutputCountInt = (int32_t) preCompOutputCnt;
      /* Update time for next invocation;
         effectively, tau is "reset" each call;
         "fracOutputsRemaining" is fraction of next output sample
         "available" at end of this input block */
      pKerPrivArgs->fracOutputsRemaining = preCompOutputCnt - (double) preCompOutputCountInt;
   }

   if (pKerPrivArgs->remBufCount > 0) {
      se3Params.ICNT0 = pKerPrivArgs->remBufCount;
      sa1Params.ICNT0 = pKerPrivArgs->remBufCount;
      __SE0_OPEN(pFilterRembufLocal, se3Params);
      __SA1_OPEN(sa1Params);

      int32_t copyLoopSize = ((pKerPrivArgs->remBufCount + eleCount - 1) / eleCount) * numChannels;

      /* copy pFilterRembufLocal to pOutLocal */
#pragma MUST_ITERATE(1, , 1)
      for (i = 0; i < copyLoopSize; i++) {
         vec remData = c7x::strm_eng<0, vec>::get_adv();

         __vpred tmp = c7x::strm_agen<1, vec>::get_vpred();
         vec    *VB1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(tmp, VB1, remData);
      }
      __SE0_CLOSE();
      __SA1_CLOSE();
      pOutLocal += pKerPrivArgs->remBufCount;
   }

   const int32_t rhoInt  = (int32_t) ROUND((double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 *
                                           (double) (rho)); /* output time incr. w.r.t. input sample */
   double        tauTemp = (double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 * (double) (tau);
   int32_t       tauInt  = (int32_t) ROUND(tauTemp); /* output time w.r.t. input sample */
   uint32_t      inputSampleIncrement =
       (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
   uint32_t nearestInputSampleIndex = inputSampleIncrement;

   vec hq0Vec, hq1Vec, hq2Vec, hq3Vec, hq4Vec, hq5Vec, hq6Vec, hq7Vec;
   vec strmFiltCoeffIn;
   vec strmSampleDataIn;

   /**********************************************************************/
   /* SA changes necessary for each exec call                               */
   /**********************************************************************/
   sa0Params.ICNT2 = preCompOutputCountInt;

   __SA0_OPEN(sa0Params);

#pragma MUST_ITERATE(1, , 1)
   for (outSampleIdx = 0; outSampleIdx < preCompOutputCountInt; outSampleIdx++) {

      k = (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_PHASE_MASK) >>
                      AUDIOLIB_ASRC_PHASE_SHIFT); /* upsampler phase */
      __SE0_OPEN(&pFiltCoeffsLocal[k * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS], se2Params);
      __SE1_OPEN(&pFiltCoeffsLocal[(k + 2) * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS], se2Params);

      const dataType dt = (dataType) (((uint32_t) tauInt & AUDIOLIB_ASRC_FRACTION_MASK)) * AUDIOLIB_ASRC_FRACTION_SCALE;
      const dataType dt2 = dt * dt;
      const dataType dt3 = dt * dt2;
      /* 3rd-order Lagrange interpolation coefficients */
      /* for use with 0 <= dt < 1 */
      // clang-format off
      const dataType q30 =     (-1.f/3) * dt + 0.5f * dt2 + (-1.f/6) * dt3;
      const dataType q31 = 1.f  - 0.5f  * dt -        dt2 +   0.5f   * dt3;
      const dataType q32 =                dt + 0.5f * dt2 -   0.5f   * dt3;
      const dataType q33 =     (-1.f/6) * dt              +  (1.f/6) * dt3;
      // clang-format on

      vec q30Vec = __duplicate(q30);
      vec q31Vec = __duplicate(q31);
      vec q32Vec = __duplicate(q32);
      vec q33Vec = __duplicate(q33);

      hq0Vec = (vec) 0;
      hq1Vec = (vec) 0;
      hq2Vec = (vec) 0;
      hq3Vec = (vec) 0;
      hq4Vec = (vec) 0;
      hq5Vec = (vec) 0;
      hq6Vec = (vec) 0;
      hq7Vec = (vec) 0;

      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q30Vec;

      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q31Vec;

      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q32Vec;

      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q33Vec;

      __SE0_CLOSE();
      __SE1_CLOSE();

      __SE0_OPEN(&pInLocal[(*cirBuffStartIndex + nearestInputSampleIndex) & (pKerPrivArgs->inBufferTotalDimX - 1)],
                 se0Params);
      __SE1_OPEN(&pInLocal2[(*cirBuffStartIndex + nearestInputSampleIndex) & (pKerPrivArgs->inBufferTotalDimX - 1)],
                 se1Params);

#pragma MUST_ITERATE(1, , 1)
      for (i = 0; i < channelLoopSize; i++) {
         // Use multiple accumulators to break the dependency chain
         vec acc0Vec = (vec) 0;
         vec acc1Vec = (vec) 0;
         vec acc2Vec = (vec) 0;
         vec acc3Vec = (vec) 0;
         vec acc4Vec = (vec) 0;
         vec acc5Vec = (vec) 0;
         vec acc6Vec = (vec) 0;
         vec acc7Vec = (vec) 0;

         // Distribute multiplications across independent accumulators
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc0Vec += strmSampleDataIn * hq0Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc1Vec += strmSampleDataIn * hq1Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc2Vec += strmSampleDataIn * hq2Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc3Vec += strmSampleDataIn * hq3Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc0Vec += strmSampleDataIn * hq4Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc1Vec += strmSampleDataIn * hq5Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc2Vec += strmSampleDataIn * hq6Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc3Vec += strmSampleDataIn * hq7Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc4Vec += strmSampleDataIn * hq0Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc5Vec += strmSampleDataIn * hq1Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc6Vec += strmSampleDataIn * hq2Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc7Vec += strmSampleDataIn * hq3Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc4Vec += strmSampleDataIn * hq4Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc5Vec += strmSampleDataIn * hq5Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc6Vec += strmSampleDataIn * hq6Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc7Vec += strmSampleDataIn * hq7Vec;
         // Combine accumulators with fewer dependencies
         acc0Vec += acc1Vec;
         acc2Vec += acc3Vec;
         acc4Vec += acc5Vec;
         acc6Vec += acc7Vec;
         vec accVec0 = acc0Vec + acc2Vec;
         vec accVec1 = acc4Vec + acc6Vec;

         /* horizontal add */
         accVec0.lo()      = accVec0.hi() + accVec0.lo();
         accVec0.lo().lo() = accVec0.lo().hi() + accVec0.lo().lo();
         dataType acc0     = (float) accVec0.s[0] + (float) accVec0.s[1];
         acc0 *= (dataType) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;
         accVec1.lo()      = accVec1.hi() + accVec1.lo();
         accVec1.lo().lo() = accVec1.lo().hi() + accVec1.lo().lo();
         dataType acc1     = (float) accVec1.s[0] + (float) accVec1.s[1];
         acc1 *= (dataType) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;

         __vpred   tmp = c7x::strm_agen<0, dataType>::get_vpred();
         dataType *VB  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
         __vstore_pred(tmp, VB, acc0);
         tmp = c7x::strm_agen<0, dataType>::get_vpred();
         VB  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
         __vstore_pred(tmp, VB, acc1);
      }

      tauInt -= (int32_t) (AUDIOLIB_ASRC_FIXEDPOINT_Q28 * inputSampleIncrement);
      tauInt += rhoInt;
      inputSampleIncrement =
          (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
      nearestInputSampleIndex += inputSampleIncrement;

      __SE0_CLOSE();
      __SE1_CLOSE();
   }

   *cirBuffStartIndex = (*cirBuffStartIndex + inputSampleCount) & (pKerPrivArgs->inBufferTotalDimX - 1);

   __SA0_CLOSE();

   // ASRC frame length must be a multiple of frameModuloFactor
   pKerOutArgs->outputSampleCount = preCompOutputCountInt + pKerPrivArgs->remBufCount;
   pKerPrivArgs->remBufCount      = pKerOutArgs->outputSampleCount % pKerPrivArgs->initArgs.frameModuloFactor;
   pKerOutArgs->outputSampleCount = pKerOutArgs->outputSampleCount - pKerPrivArgs->remBufCount;

   if (pKerPrivArgs->remBufCount > 0) {
      se4Params.ICNT0 = pKerPrivArgs->remBufCount;
      sa2Params.ICNT0 = pKerPrivArgs->remBufCount;

      __SE0_OPEN((dataType *) pOut + pKerOutArgs->outputSampleCount, se4Params);
      __SA1_OPEN(sa2Params);

      int32_t copyLoopSize = ((pKerPrivArgs->remBufCount + eleCount - 1) / eleCount) * numChannels;

      /* copy pOutLocal to pFilterRembufLocal  */
#pragma MUST_ITERATE(1, , 1)
      for (i = 0; i < copyLoopSize; i++) {
         vec remData = c7x::strm_eng<0, vec>::get_adv();

         __vpred tmp = c7x::strm_agen<1, vec>::get_vpred();
         vec    *VB1 = c7x::strm_agen<1, vec>::get_adv(pFilterRembufLocal);
         __vstore_pred(tmp, VB1, remData);
      }
      __SE0_CLOSE();
      __SA1_CLOSE();
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_interleaved(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pNonInterleavedData,
                                                  void *restrict pFiltCoeffs,
                                                  void *restrict pFilterRembuf,
                                                  void *restrict pOut,
                                                  const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                  AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_exec_ci_interleaved\n");
#endif

   AUDIOLIB_STATUS         status                                = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplib_status __attribute__((unused)) = DSPLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs                          = (AUDIOLIB_asrc_PrivArgs *) handle;
   uint8_t                *pBlock                                = pKerPrivArgs->bufPblock;
   uint32_t               *cirBuffStartIndex                     = &pKerPrivArgs->cirBuffStartIndex;
   dataType               *pFilterRembufLocal                    = (dataType *) pFilterRembuf;
   const dataType         *pFiltCoeffsLocal                      = (dataType *) pFiltCoeffs;
   dataType               *pNonInterleavedDataLocal              = (dataType *) pNonInterleavedData;
   dataType               *pNonInterleavedDataLocal2 = pNonInterleavedDataLocal + pKerPrivArgs->inBufferTotalDimX;
   dataType               *pOutLocal                 = (dataType *) pOut;
   const uint8_t           numChannels               = pKerPrivArgs->initArgs.numChannels;
   int32_t                 channelLoopSize           = pKerPrivArgs->channelLoopSize;
   uint32_t                inputSampleCount          = pKerInArgs->inputSampleCount;
   int32_t                 preCompOutputCountInt;
   int32_t                 i, k, outSampleIdx;

   // Type definition for vector operations
   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   // output / input time step
   /* maxOutputsPerInputRatio will set the maximum rate ratio change acceptable*/
   /** Accumulator increment value. This variable stores the accumulator increment value, which is calculated as
    * 1/asrcRatio.
    */
   const double rho = 1 / AUDIOLIB_ASRC_MIN(pKerPrivArgs->outputsPerInputRatio, pKerPrivArgs->maxOutputsPerInputRatio);
   /** Accumulator value. This variable stores the accumulator value used in the ARC2 core processing.
    */
   const double tau = rho * (1 - pKerPrivArgs->fracOutputsRemaining);

   // Use rho directly without storing it in pKerPrivArgs->rho
   double preCompOutputCnt = ((double) pKerInArgs->inputSampleCount / rho) + pKerPrivArgs->fracOutputsRemaining;
   /* This condition check has to be done to prevent the algorithm from interpolating the last output sample without
    * an full input sample. Ex:- for 32 kHz -> 48 kHz example, Assume input sample count = 256 so precomputer output
    * sample count = 256 * 48/32 = 384 samples But if you try to generate 384 samples in the algorithm, it'll
    * interpolate the last output sample without an input sample because it has run out of all 256 input samples.
    * This 384th output sample perfectly aligns with the 257th input sample (if we have one!). So if this perfect
    * alignment happens we need to reduce 1 output sample. Which means fracOutputsRemaining will be 1;
    */
   if (preCompOutputCnt == (int32_t) preCompOutputCnt) {
      preCompOutputCnt--;
      preCompOutputCountInt              = (int32_t) preCompOutputCnt;
      pKerPrivArgs->fracOutputsRemaining = 1.0;
   }
   else {
      preCompOutputCountInt = (int32_t) preCompOutputCnt;
      /* Update time for next invocation;
         effectively, tau is "reset" each call;
         "fracOutputsRemaining" is fraction of next output sample
         "available" at end of this input block */
      pKerPrivArgs->fracOutputsRemaining = preCompOutputCnt - (double) preCompOutputCountInt;
   }

   memcpy(pOutLocal, pFilterRembufLocal,
          numChannels * pKerPrivArgs->remBufCount * AUDIOLIB_sizeof(pKerPrivArgs->bufParamsFilterRembuf.data_type));

   pOutLocal += pKerPrivArgs->remBufCount * (int32_t) numChannels;

   const int32_t rhoInt  = (int32_t) ROUND((double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 *
                                           (double) (rho)); /* output time incr. w.r.t. input sample */
   double        tauTemp = (double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 * (double) (tau);
   int32_t       tauInt  = (int32_t) ROUND(tauTemp); /* output time w.r.t. input sample */
   uint32_t      inputSampleIncrement =
       (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
   uint32_t nearestInputSampleIndex = inputSampleIncrement;

   vec hq0Vec, hq1Vec, hq2Vec, hq3Vec, hq4Vec, hq5Vec, hq6Vec, hq7Vec;
   vec strmFiltCoeffIn;
   vec strmSampleDataIn;

   dsplib_status = DSPLIB_matTrans_exec(
       pKerPrivArgs->initArgs.matTransHandle, pIn,
       (void *) (&pNonInterleavedDataLocal[(*cirBuffStartIndex +
                                            (int32_t) (AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS - 1)) &
                                           (pKerPrivArgs->inBufferTotalDimX - 1)]));

   /**********************************************************************/
   /* SA changes necessary for each exec call                               */
   /**********************************************************************/
   sa0Params.ICNT2 = preCompOutputCountInt;

   __SA0_OPEN(sa0Params);

#pragma MUST_ITERATE(1, , 1)
   for (outSampleIdx = 0; outSampleIdx < preCompOutputCountInt; outSampleIdx++) {

      k = (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_PHASE_MASK) >>
                      AUDIOLIB_ASRC_PHASE_SHIFT); /* upsampler phase */
      __SE0_OPEN(&pFiltCoeffsLocal[k * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS], se2Params);
      __SE1_OPEN(&pFiltCoeffsLocal[(k + 2) * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS], se2Params);

      const dataType dt = (dataType) (((uint32_t) tauInt & AUDIOLIB_ASRC_FRACTION_MASK)) * AUDIOLIB_ASRC_FRACTION_SCALE;
      const dataType dt2 = dt * dt;
      const dataType dt3 = dt * dt2;
      /* 3rd-order Lagrange interpolation coefficients */
      /* for use with 0 <= dt < 1 */
      // clang-format off
         const dataType q30 =     (-1.f/3) * dt + 0.5f * dt2 + (-1.f/6) * dt3;
         const dataType q31 = 1.f  - 0.5f  * dt -        dt2 +   0.5f   * dt3;
         const dataType q32 =                dt + 0.5f * dt2 -   0.5f   * dt3;
         const dataType q33 =     (-1.f/6) * dt              +  (1.f/6) * dt3;
      // clang-format on

      vec q30Vec = __duplicate(q30);
      vec q31Vec = __duplicate(q31);
      vec q32Vec = __duplicate(q32);
      vec q33Vec = __duplicate(q33);

      hq0Vec = (vec) 0;
      hq1Vec = (vec) 0;
      hq2Vec = (vec) 0;
      hq3Vec = (vec) 0;
      hq4Vec = (vec) 0;
      hq5Vec = (vec) 0;
      hq6Vec = (vec) 0;
      hq7Vec = (vec) 0;

      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q30Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q30Vec;

      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q31Vec;
      strmFiltCoeffIn = c7x::strm_eng<0, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q31Vec;

      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q32Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q32Vec;

      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq0Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq1Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq2Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq3Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq4Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq5Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq6Vec += strmFiltCoeffIn * q33Vec;
      strmFiltCoeffIn = c7x::strm_eng<1, vec>::get_adv();
      hq7Vec += strmFiltCoeffIn * q33Vec;

      __SE0_CLOSE();
      __SE1_CLOSE();

      __SE0_OPEN(&pNonInterleavedDataLocal[(*cirBuffStartIndex + nearestInputSampleIndex) &
                                           (pKerPrivArgs->inBufferTotalDimX - 1)],
                 se0Params);
      __SE1_OPEN(&pNonInterleavedDataLocal2[(*cirBuffStartIndex + nearestInputSampleIndex) &
                                            (pKerPrivArgs->inBufferTotalDimX - 1)],
                 se1Params);

#pragma MUST_ITERATE(1, , 1)
      for (i = 0; i < channelLoopSize; i++) {
         // Use multiple accumulators to break the dependency chain
         vec acc0Vec = (vec) 0;
         vec acc1Vec = (vec) 0;
         vec acc2Vec = (vec) 0;
         vec acc3Vec = (vec) 0;
         vec acc4Vec = (vec) 0;
         vec acc5Vec = (vec) 0;
         vec acc6Vec = (vec) 0;
         vec acc7Vec = (vec) 0;

         // Distribute multiplications across independent accumulators
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc0Vec += strmSampleDataIn * hq0Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc1Vec += strmSampleDataIn * hq1Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc2Vec += strmSampleDataIn * hq2Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc3Vec += strmSampleDataIn * hq3Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc0Vec += strmSampleDataIn * hq4Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc1Vec += strmSampleDataIn * hq5Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc2Vec += strmSampleDataIn * hq6Vec;
         strmSampleDataIn = c7x::strm_eng<0, vec>::get_adv();
         acc3Vec += strmSampleDataIn * hq7Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc4Vec += strmSampleDataIn * hq0Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc5Vec += strmSampleDataIn * hq1Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc6Vec += strmSampleDataIn * hq2Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc7Vec += strmSampleDataIn * hq3Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc4Vec += strmSampleDataIn * hq4Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc5Vec += strmSampleDataIn * hq5Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc6Vec += strmSampleDataIn * hq6Vec;
         strmSampleDataIn = c7x::strm_eng<1, vec>::get_adv();
         acc7Vec += strmSampleDataIn * hq7Vec;
         // Combine accumulators with fewer dependencies
         acc0Vec += acc1Vec;
         acc2Vec += acc3Vec;
         acc4Vec += acc5Vec;
         acc6Vec += acc7Vec;
         vec accVec0 = acc0Vec + acc2Vec;
         vec accVec1 = acc4Vec + acc6Vec;

         /* horizontal add */
         accVec0.lo()      = accVec0.hi() + accVec0.lo();
         accVec0.lo().lo() = accVec0.lo().hi() + accVec0.lo().lo();
         dataType acc0     = (float) accVec0.s[0] + (float) accVec0.s[1];
         acc0 *= (dataType) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;
         accVec1.lo()      = accVec1.hi() + accVec1.lo();
         accVec1.lo().lo() = accVec1.lo().hi() + accVec1.lo().lo();
         dataType acc1     = (float) accVec1.s[0] + (float) accVec1.s[1];
         acc1 *= (dataType) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;

         __vpred   tmp = c7x::strm_agen<0, dataType>::get_vpred();
         dataType *VB  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
         __vstore_pred(tmp, VB, acc0);
         tmp = c7x::strm_agen<0, dataType>::get_vpred();
         VB  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
         __vstore_pred(tmp, VB, acc1);
      }

      tauInt -= (int32_t) (AUDIOLIB_ASRC_FIXEDPOINT_Q28 * inputSampleIncrement);
      tauInt += rhoInt;
      inputSampleIncrement =
          (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
      nearestInputSampleIndex += inputSampleIncrement;

      __SE0_CLOSE();
      __SE1_CLOSE();
   }

   *cirBuffStartIndex = (*cirBuffStartIndex + inputSampleCount) & (pKerPrivArgs->inBufferTotalDimX - 1);

   __SA0_CLOSE();

   // ASRC frame length must be a multiple of frameModuloFactor
   pKerOutArgs->outputSampleCount = preCompOutputCountInt + pKerPrivArgs->remBufCount;
   pKerPrivArgs->remBufCount      = pKerOutArgs->outputSampleCount % pKerPrivArgs->initArgs.frameModuloFactor;
   pKerOutArgs->outputSampleCount = pKerOutArgs->outputSampleCount - pKerPrivArgs->remBufCount;

   memcpy(pFilterRembufLocal, (dataType *) pOut + pKerOutArgs->outputSampleCount * numChannels,
          numChannels * pKerPrivArgs->remBufCount * AUDIOLIB_sizeof(pKerPrivArgs->bufParamsFilterRembuf.data_type));

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_non_interleaved<float>(AUDIOLIB_kernelHandle handle,
                                                                      void *restrict pIn,
                                                                      void *restrict pNonInterleavedData,
                                                                      void *restrict pFiltCoeffs,
                                                                      void *restrict pFilterRembuf,
                                                                      void *restrict pOut,
                                                                      const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                                      AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

template AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_interleaved<float>(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pNonInterleavedData,
                                                                  void *restrict pFiltCoeffs,
                                                                  void *restrict pFilterRembuf,
                                                                  void *restrict pOut,
                                                                  const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                                  AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);
