// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_AGC_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_AGC_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_agc.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_agc_PrivArgs structure.
 */
#define AUDIOLIB_AGC_IXX_IXX_OXX_PBLOCK_SIZE (4 * SE_PARAM_SIZE)

/*!
 * @brief Maximum number of elements in a C7x vector (float on 512-bit SIMD).
 */
#define AUDIOLIB_AGC_MAX_VEC_ELEMENTS (16)

/*!
 * @brief Number of vectors processed per iteration of the C7x energy (per stream) and gain apply loops.
 */
#define AUDIOLIB_AGC_UNROLL (4)

#define AUDIOLIB_AGC_MS_PER_SECOND (1000.0f)

/** @brief Linear gain of 0 dB, the initial target gain and the recovery end point */
#define AUDIOLIB_AGC_UNITY_GAIN (1.0f)

/** @brief Bounds of @ref AUDIOLIB_agc_InitArgs checked by @ref AUDIOLIB_agc_init_checkParams */
#define AUDIOLIB_AGC_SAMPLING_RATE_LOWER_BOUND (0)      // Exclusive: samplingRate must be > 0 Hz

/** @brief Bounds of @ref AUDIOLIB_agc_SetArgs checked by @ref AUDIOLIB_agc_set */
#define AUDIOLIB_AGC_SMOOTHING_TIME_LOWER_BOUND (0.0f)  // Exclusive: smoothingTime must be > 0 msec
#define AUDIOLIB_AGC_MIN_RATIO (1.0f)                   // Inclusive: ratio of 1 dB/dB applies no compression
#define AUDIOLIB_AGC_MIN_MAX_GAIN_DB (0.0f)             // Inclusive: maxGain must be >= 0 dB
#define AUDIOLIB_AGC_MIN_MAX_ATTENUATION_DB (0.0f)      // Inclusive: maxAttenuation must be >= 0 dB
#define AUDIOLIB_AGC_MIN_RECOVERY_RATE (0.0f)           // Inclusive: recoveryRate must be >= 0 dB/sec

/*
 * Cycle model of AUDIOLIB_agc_exec_ci used by AUDIOLIB_agc_perfEst:
 *   est  = ENTRY + ENERGY_TO_GAIN + OUTPUT_SETUP + CONTROL + TEARDOWN + arch
 *   arch = ENERGY_LOOP_OVERHEAD + (ENERGY_LOOP_II * nIterIn) +
 *          nRowsOut * (ROW_LOOP_CONTROL + OUTPUT_LOOP_PROLOG + (OUTPUT_LOOP_II * nIterPerRowOut) + OUTPUT_LOOP_EPILOG)
 */
#define AUDIOLIB_AGC_ACTIVE_CONTROL_CYCLES (135)  // Target gain update above activationThreshold (log2/exp2 path)
#define AUDIOLIB_AGC_INACTIVE_CONTROL_CYCLES (33) // Target gain update below activationThreshold (hold/recovery path)

#define AUDIOLIB_AGC_SP_ENTRY_CYCLES (28)              // FP32: function entry, template loads and SE0/SE1 open
#define AUDIOLIB_AGC_SP_ENERGY_TO_GAIN_CYCLES (53)     // FP32: SE0/SA0 reopen and horizontal add of the accumulators
#define AUDIOLIB_AGC_SP_OUTPUT_SETUP_CYCLES (16)       // FP32: gain chain seed vectors before the output pass
#define AUDIOLIB_AGC_SP_TEARDOWN_CYCLES (15)           // FP32: SE0/SA0 close, current gain update and return
#define AUDIOLIB_AGC_SP_ENERGY_LOOP_OVERHEAD_CYCLES (7) // FP32: energy loop prolog and epilog
#define AUDIOLIB_AGC_SP_ENERGY_LOOP_II (4)             // FP32: energy loop ii
#define AUDIOLIB_AGC_SP_ROW_LOOP_CONTROL_CYCLES (3)    // FP32: output row loop control
#define AUDIOLIB_AGC_SP_OUTPUT_LOOP_PROLOG_CYCLES (7)  // FP32: output inner loop prolog
#define AUDIOLIB_AGC_SP_OUTPUT_LOOP_II (4)             // FP32: output inner loop ii
#define AUDIOLIB_AGC_SP_OUTPUT_LOOP_EPILOG_CYCLES (3)  // FP32: output inner loop epilog

#define AUDIOLIB_AGC_DP_ENTRY_CYCLES (29)              // FP64: function entry, template loads and SE0/SE1 open
#define AUDIOLIB_AGC_DP_ENERGY_TO_GAIN_CYCLES (53)     // FP64: SE0/SA0 reopen and horizontal add of the accumulators
#define AUDIOLIB_AGC_DP_OUTPUT_SETUP_CYCLES (17)       // FP64: gain chain seed vectors before the output pass
#define AUDIOLIB_AGC_DP_TEARDOWN_CYCLES (18)           // FP64: SE0/SA0 close, current gain update and return
#define AUDIOLIB_AGC_DP_ENERGY_LOOP_OVERHEAD_CYCLES (7) // FP64: energy loop prolog and epilog
#define AUDIOLIB_AGC_DP_ENERGY_LOOP_II (8)             // FP64: energy loop ii
#define AUDIOLIB_AGC_DP_ROW_LOOP_CONTROL_CYCLES (3)    // FP64: output row loop control
#define AUDIOLIB_AGC_DP_OUTPUT_LOOP_PROLOG_CYCLES (4)  // FP64: output inner loop prolog
#define AUDIOLIB_AGC_DP_OUTPUT_LOOP_II (8)             // FP64: output inner loop ii
#define AUDIOLIB_AGC_DP_OUTPUT_LOOP_EPILOG_CYCLES (4)  // FP64: output inner loop epilog

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_agc_exec_ci
 *         and @ref AUDIOLIB_agc_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_agc_exec)(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn,
                                                     void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_agc_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and stream address generator, and the configuration is saved in
 *          bufPBlock array.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_agc_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_agc_InitArgs *pKerInitArgs);

/*!
 *  @brief This function computes the C7x specific derived parameters from the
 *         set parameters. It is called by @ref AUDIOLIB_agc_set.
 *
 * @details This function precomputes the vector of gain smoothing coefficients
 *          and the linear domain threshold and gain limits used by
 *          @ref AUDIOLIB_agc_exec_ci.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  pKerSetArgs   :  Pointer to the structure holding set parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_agc_set_ci(AUDIOLIB_kernelHandle handle, const AUDIOLIB_agc_SetArgs *pKerSetArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_agc_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_agc_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_agc_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_agc_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_agc_exec_cn and
    *         @ref AUDIOLIB_agc_exec_ci.                        */
   pFxnAUDIOLIB_agc_exec execute;

   AUDIOLIB_data_type_e dataType; /**< \brief Data type (AUDIOLIB_FLOAT32 or AUDIOLIB_FLOAT64) */

   uint32_t dim_x;     /**< \brief Number of samples per channel. */
   uint32_t dim_y;     /**< \brief Number of channels. */
   int32_t  strideIn;  /**< \brief Input stride in Y dimension in elements. */
   int32_t  strideOut; /**< \brief Output stride in Y dimension in elements. */

   AUDIOLIB_agc_InitArgs initArgs; /**< \brief A copy of the initialization parameters. */
   AUDIOLIB_agc_SetArgs  setArgs;  /**< \brief A copy of the last-set runtime parameters. */

   float oneOverSlope;         /**< \brief 1 - 1/ratio */
   float smoothingCoeffSample; /**< \brief Per-sample gain smoothing coefficient */
   float smoothingCoeffBlock;  /**< \brief Per-block energy smoothing coefficient */
   float recoveryRateUp;       /**< \brief Per-block gain multiplier when recovering up to 0 dB */
   float recoveryRateDown;     /**< \brief Per-block gain multiplier when recovering down to 0 dB */
   float oneOverNumSamples;    /**< \brief 1 / (dim_x * dim_y) */
   float blockDecay;           /**< \brief (1 - smoothingCoeffSample) ^ (dim_x * dim_y) */

   float currentGain; /**< \brief Linear output gain of the last sample */
   float targetGain;  /**< \brief Linear gain the output is smoothed towards */
   float energy;      /**< \brief Smoothed mean energy of the input */

   uint32_t gainComputed; /**< \brief 1 if the last block recomputed the target gain (level above threshold), else 0 */

   float thrEnergy; /**< \brief 10^(activationThreshold/10), activation threshold as energy (C7x) */
   float gainScale; /**< \brief 10^(targetLevel * oneOverSlope / 20), linear gain at unit energy (C7x) */
   float gainExp;   /**< \brief -oneOverSlope / 2, exponent applied to the energy (C7x) */
   float gainMin;   /**< \brief 10^(-maxAttenuation/20), linear lower gain limit (C7x) */
   float gainMax;   /**< \brief 10^(maxGain/20), linear upper gain limit (C7x) */

   uint32_t
       nIterIn; /**< \brief Energy pass iterations, AUDIOLIB_AGC_UNROLL vectors from each of SE0 and SE1 (C7x) */
   int32_t
       in1Offset; /**< \brief Offset in elements from pIn to the first sample read by SE1 in the energy pass (C7x) */
   uint32_t nRowsOut; /**< \brief Rows walked by the output pass: 1 when the gain chain is continuous, dim_y otherwise
                         (C7x) */
   uint32_t nIterPerRowOut; /**< \brief Output pass iterations of AUDIOLIB_AGC_UNROLL vectors per row (C7x) */

   /*! @brief (1 - smoothingCoeffSample)^(i+1) for the lanes of the first AUDIOLIB_AGC_UNROLL output
    *         vectors, stored as the kernel data type (C7x) */
   double decayPow[AUDIOLIB_AGC_UNROLL * AUDIOLIB_AGC_MAX_VEC_ELEMENTS];
   /*! @brief (1 - smoothingCoeffSample)^(AUDIOLIB_AGC_UNROLL * vector length), advances the gain
    *         coefficients by one loop iteration, stored as the kernel data type (C7x) */
   double decayPowUnroll;
   /*! @brief (1 - smoothingCoeffSample)^dim_x, advances the gain coefficients by one row, stored as the kernel
    *         data type (C7x) */
   double decayPowRow;

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_AGC_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_agc_PrivArgs;

/*!
 *  @brief Block-rate update of the smoothed energy and the target gain.
 *
 *  @param [in,out] pKerPrivArgs : Kernel private arguments
 *  @param [in]     energySum    : Sum of squares of all input samples in the block
 */
void AUDIOLIB_agc_updateTargetGain(AUDIOLIB_agc_PrivArgs *pKerPrivArgs, float energySum);

#endif /* AUDIOLIB_AGC_IXX_IXX_OXX_PRIV_H_ */
