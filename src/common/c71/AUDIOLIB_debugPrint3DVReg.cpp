// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../AUDIOLIB_bufParams.h"
#include "../AUDIOLIB_types.h"

/******************************************************************************/

/******************************************************************************/
/*!
 * \ingroup
 * \brief
 * \details
 * \return  void
 */
/******************************************************************************/

template <typename dataType>
void AUDIOLIB_debugPrint3DVReg_helper(dataType *matrix, const AUDIOLIB_bufParams3D_t *params)
{
   uint32_t  r, c, t;
   dataType *xPtr;
   dataType *yPtr;
   dataType *zPtr = matrix;

   // stride_y is stored in bytes, but easier to use in elements
   uint32_t stride_y_elements = params->stride_y / AUDIOLIB_sizeof(params->data_type);
   uint32_t stride_z_elements = params->stride_z / AUDIOLIB_sizeof(params->data_type);

   AUDIOLIB_PRINTF("%s", "{\n");
   for (t = 0; t < params->dim_z; t++) {
      yPtr = zPtr;
      for (r = 0; r < params->dim_y; r++) {
         xPtr = yPtr;
         for (c = 0; c < params->dim_x; c++) {
            AUDIOLIB_PRINTF("%3d, ", *(xPtr));
            xPtr++;
         }
         AUDIOLIB_PRINTF("%s", "\n");
         yPtr += stride_y_elements;
      }
      zPtr += stride_z_elements;
   }
   AUDIOLIB_PRINTF("%s", "};\n");
   return;
}

template void AUDIOLIB_debugPrint3DVReg_helper<int8_t>(int8_t *matrix, const AUDIOLIB_bufParams3D_t *params);
template void AUDIOLIB_debugPrint3DVReg_helper<int16_t>(int16_t *matrix, const AUDIOLIB_bufParams3D_t *params);
template void AUDIOLIB_debugPrint3DVReg_helper<uint16_t>(uint16_t *matrix, const AUDIOLIB_bufParams3D_t *params);
template void AUDIOLIB_debugPrint3DVReg_helper<int32_t>(int32_t *matrix, const AUDIOLIB_bufParams3D_t *params);

/******************************************************************************/

/******************************************************************************/
/*!
 * \ingroup
 * \brief
 * \details
 * \return  void
 */
/******************************************************************************/

// want this function to have C-linkage in library...
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void AUDIOLIB_debugPrint3DVReg(void *matrix, const AUDIOLIB_bufParams3D_t *params)
{
   switch (params->data_type) {
   case AUDIOLIB_INT32:
      AUDIOLIB_debugPrint3DVReg_helper<int32_t>((int32_t *) matrix, params);
      break;
   case AUDIOLIB_INT16:
      AUDIOLIB_debugPrint3DVReg_helper<int16_t>((int16_t *) matrix, params);
      break;
   case AUDIOLIB_UINT16:
      AUDIOLIB_debugPrint3DVReg_helper<uint16_t>((uint16_t *) matrix, params);
      break;
   case AUDIOLIB_INT8:
      AUDIOLIB_debugPrint3DVReg_helper<int8_t>((int8_t *) matrix, params);
      break;
   default:
      AUDIOLIB_PRINTF("\nERROR: Unrecognized data type in %s.\n", __FUNCTION__);
   }

   return;
}
#ifdef __cplusplus
}
#endif /* __cplusplus */
