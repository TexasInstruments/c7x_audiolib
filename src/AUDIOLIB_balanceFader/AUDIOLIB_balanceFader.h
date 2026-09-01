// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_H_
#define AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_balanceFader AUDIOLIB_balanceFader
 * @brief Kernel for applying balance and fader controls to multichannel audio.
 *
 * @details
 * This kernel calculates individual gain values for each audio channel based on
 * left/right balance and front/rear fader controls. It supports complex speaker
 * configurations, including side, top, and LFE (subwoofer) channels with
 * specialized behavior.
 *
 * The `_set` function calculates an array of gains based on the provided settings.
 * The `_exec` function then applies these pre-calculated gains to the audio stream,
 * typically by calling a dedicated multichannel gain kernel.
 *
 * ---
 * ### Gain Calculation Logic
 *
 * The final gain for each speaker is determined by combining fundamental gain
 * components derived from the `balance` (b) and `fade` (f) parameters.
 *
 * #### 1. Fundamental Gain Components
 * Both balance and fader controls use a constant-power panning law.
 *
 * - **Balance Components** (Left/Right):
 * \f[ \theta_b = \frac{\pi}{4}(b+1) \f]
 * \f[ G_L = \cos(\theta_b) \quad | \quad G_R = \sin(\theta_b) \f]
 *
 * - **Fader Components** (Front/Rear):
 * \f[ \theta_f = \frac{\pi}{4}(f+1) \f]
 * \f[ G_{front} = \sin(\theta_f) \quad | \quad G_{rear} = \cos(\theta_f) \f]
 *
 * #### 2. Derived Gain Components
 * Gains for specific speaker locations are derived from the fundamental components.
 *
 * - **Side Speaker Gain** controlled by `sideGainFactor`:
 * - If fading to rear (f < 0): \f[ G_{side} = g_{side} \cdot G_{rear} + (1 - g_{side}) \cdot G_{front} \f]
 * - If fading to front (f >= 0): \f[ G_{side} = G_{rear} \f]
 *
 * - **Top Middle Speaker Gain**:
 * \f[ G_{top\_middle} = 0.5 \cdot (G_{front} + G_{rear}) \f]
 *
 * #### 3. Per-Channel Final Gain
 * The final gain for a standard channel is the product of its balance and fader components.
 * For example:
 * - **Front Left Gain:** \f[ \text{Gain}_{FL} = G_L \cdot G_{front} \f]
 * - **Rear Right Gain:** \f[ \text{Gain}_{RR} = G_R \cdot G_{rear} \f]
 * - **Side Left Gain:** \f[ \text{Gain}_{SL} = G_L \cdot G_{side} \f]
 *
 * #### 4. LFE (Subwoofer) Channel Gain
 * The LFE gain is calculated as: \f$\text{G}_{LFE} = G_{LFE,b} \cdot G_{LFE,f}\f$ which are determined by the
 * `lfeBalanceMode` and `lfeFaderMode` enums.
 *
 * - **LFE Balance Component:**
 * - `UNAFFECTED`: \f[ G_{LFE,b} = 1.0 \f]
 * - `PARTIAL`: \f[ G_{LFE,b} = g_{b,lfe} + (1 - g_{b,lfe}) \cdot \cos(2\theta_b) \f]
 * - `FULL`: \f[ G_{LFE,b} = 0.5 \cdot (G_L + G_R) \f]
 *
 * - **LFE Fader Component:**
 * - `UNAFFECTED`: \f[ G_{LFE,f} = 1.0 \f]
 * - `AVERAGE`: \f[ G_{LFE,f} = 0.5 \cdot (G_{front} + G_{rear}) \f]
 * - `POSITION_BASED`: \f[ G_{LFE,f} = (1 - g_{f,lfe}) \cdot G_{front} + g_{f,lfe} \cdot G_{rear} \f]
 * - `FULL`: \f[ G_{LFE,f} = G_{rear} \f]
 *
 * The final LFE gain is: \f[ G_{LFE} = G_{LFE,b} \cdot G_{LFE,f} \f]
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Enum defining how the LFE (subwoofer) channel reacts to balance controls.
 */
typedef enum {
   AUDIOLIB_LFE_BALANCE_UNAFFECTED = 0, /**< LFE gain is not affected by the balance setting */
   AUDIOLIB_LFE_BALANCE_PARTIAL,        /**< LFE gain is partially affected by the balance setting.*/
   AUDIOLIB_LFE_BALANCE_FULL            /**< LFE gain is fully panned left and right like a standard speaker*/
} AUDIOLIB_LFE_BALANCE_MODE;

/**
 * @brief Enum defining how the LFE (subwoofer) channel reacts to fader controls.
 */
typedef enum {
   AUDIOLIB_LFE_FADER_UNAFFECTED = 0, /**< LFE gain is not affected by the fader setting */
   AUDIOLIB_LFE_FADER_AVERAGE,        /**< LFE gain is an average of the front and back gains */
   AUDIOLIB_LFE_FADER_POSITION_BASED, /**< LFE gain is weighted towards the front or back, simulating a physical
                                         position. */
   AUDIOLIB_LFE_FADER_FULL            /**< LFE gain is treated as a standard rear speaker */
} AUDIOLIB_LFE_FADER_MODE;

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t  funcStyle;
   uint8_t isInterleave; /**< Interleave flag. */
} AUDIOLIB_balanceFader_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {
   float    balance;              /**< Left/Right control. Range: -1.0 (full left) to 1.0 (full right). */
   float    fade;                 /**< Front/Rear control. Range: -1.0 (full rear) to 1.0 (full front). */
   int32_t *channelConfig;        /**< Pointer to an integer array defining the speaker type for each channel. */
   float    lfeBalanceGainFactor; /**< LFE balance gain factor. Range: 0.0 (no effect) to 1.0 (full effect). */
   float    lfeFaderGainFactor;   /**< LFE fader gain factor. Range: 0.0 (no effect) to 1.0 (full effect). */
   float    sideGainFactor;       /**< Side gain factor. Range: 0.0 to 1.0. */
   int32_t  lfeBalanceMode;       /**< The selected mode for how the LFE channel responds to the balance control. */
   int32_t  lfeFaderMode;         /**< The selected mode for how the LFE channel responds to the fader control. */
} AUDIOLIB_balanceFader_SetArgs;
/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_balanceFader_getHandleSize(AUDIOLIB_balanceFader_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_balanceFader_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of balanceFader gains
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */

AUDIOLIB_STATUS AUDIOLIB_balanceFader_init(AUDIOLIB_kernelHandle                 handle,
                                           const AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                           const AUDIOLIB_bufParams1D_t         *bufParamsGain,
                                           const AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                           const AUDIOLIB_balanceFader_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_balanceFader_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_balanceFader_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_balanceFader_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional information of gain buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_balanceFader_init_checkParams(AUDIOLIB_kernelHandle                 handle,
                                       const AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                       const AUDIOLIB_bufParams1D_t         *bufParamsGain,
                                       const AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                       const AUDIOLIB_balanceFader_InitArgs *pKerInitArgs);

/**
 * @brief Sets the balanceFader parameters for the audio processing kernel.
 *
 * @param handle Active handle to the kernel.
 * @param pGain Pointer to the buffer holding the Gain buffer
 * @param pKerSetArgs Pointer to the structure containing balanceFader parameters
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */

AUDIOLIB_STATUS AUDIOLIB_balanceFader_set(AUDIOLIB_kernelHandle handle,
                                          void *restrict pGain,
                                          AUDIOLIB_balanceFader_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the initial balanceFader parameters from the audio processing kernel.
 *
 * @param handle Active handle to the kernel.
 * @param pKerSetArgs Pointer to the structure where the current balanceFader parameters
 *        will be stored, including balanceFader level, sampling rate, and smoothing time.
 *
 * @details This function copies the internal state of the kernel's balanceFader settings
 *          into the provided `pKerSetArgs` structure. It allows the application to
 *          query the current balanceFader configuration.
 *
 * @return Status value indicating success. Refer to @ref AUDIOLIB_STATUS.
 */

AUDIOLIB_STATUS AUDIOLIB_balanceFader_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_balanceFader_SetArgs *pKerSetArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_balanceFader_exec function. \
 *               This function must be called before the @ref AUDIOLIB_balanceFader_exec is called.
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the  input buffer
 *  @param [in]  pGain      : Pointer to buffer holding the Gain buffer
 *  @param [in]  pOut       : Pointer to buffer holding the  output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 * */

AUDIOLIB_STATUS AUDIOLIB_balanceFader_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                       const void *restrict pIn,
                                                       const void *restrict pGain,
                                                       const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_balanceFader_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the  input buffer
 *  @param [in]  pGain      : Pointer to buffer holding the Gain buffer
 *  @param [in]  pOut       : Pointer to buffer holding the  output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - None
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_balanceFader_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_balanceFader_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch compute cycles obtained from asm
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *  @remarks     None
 */
void AUDIOLIB_balanceFader_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_H_ */
