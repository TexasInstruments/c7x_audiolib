// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../AUDIOLIB_types.h"
#include "AUDIOLIB_debug.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************************/

/******************************************************************************/
/*!
 * \ingroup
 * \brief
 * \details
 * \return  void
 */
/******************************************************************************/

void AUDIOLIB_debugPrintBufParams1D(const AUDIOLIB_bufParams1D_t *params)
{
   AUDIOLIB_PRINTF("   dim_x        = %d\n", params->dim_x);
   AUDIOLIB_PRINTF("   data_type    = %d\n", params->data_type);

   return;
}

void AUDIOLIB_debugPrintBufParams(const AUDIOLIB_bufParams2D_t *params)
{
   AUDIOLIB_PRINTF("   dim_x        = %d\n", params->dim_x);
   AUDIOLIB_PRINTF("   dim_y        = %d\n", params->dim_y);
   AUDIOLIB_PRINTF("   data_type    = %d\n", params->data_type);
   AUDIOLIB_PRINTF("   stride_y     = %d\n", params->stride_y);

   return;
}

void AUDIOLIB_debugPrintBufParams3D(const AUDIOLIB_bufParams3D_t *params)
{
   AUDIOLIB_PRINTF("   dim_x        = %d\n", params->dim_x);
   AUDIOLIB_PRINTF("   dim_y        = %d\n", params->dim_y);
   AUDIOLIB_PRINTF("   dim_z        = %d\n", params->dim_z);
   AUDIOLIB_PRINTF("   data_type    = %d\n", params->data_type);
   AUDIOLIB_PRINTF("   stride_y     = %d\n", params->stride_y);
   AUDIOLIB_PRINTF("   stride_z     = %d\n", params->stride_z);

   return;
}
#ifdef __cplusplus
}
#endif /* __cplusplus */
