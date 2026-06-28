// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../AUDIOLIB_bufParams.h"
#include "../AUDIOLIB_types.h"
#include "../AUDIOLIB_utility.h"

template <typename dataType>
void AUDIOLIB_debugPrintMatrix3D_helper(dataType *matrix, const AUDIOLIB_bufParams3D_t *params)
{
   uint32_t  x, y, z;
   dataType *xPtr;
   dataType *yPtr;
   dataType *zPtr = matrix;

   // stride_y is stored in bytes, but easier to use in elements
   uint32_t stride_y_elements = params->stride_y / AUDIOLIB_sizeof(params->data_type);
   uint32_t stride_z_elements = params->stride_z / AUDIOLIB_sizeof(params->data_type);

   for (z = 0; z < params->dim_z; z++) {
      AUDIOLIB_PRINTF("%s", "\n");
      yPtr = zPtr;
      for (y = 0; y < params->dim_y; y++) {
         xPtr = yPtr;
         AUDIOLIB_PRINTF("%p |", xPtr);
         for (x = 0; x < params->dim_x; x++) {
            AUDIOLIB_PRINTF("%3d ", *(xPtr));
            xPtr++;
         }
         AUDIOLIB_PRINTF("%s", "|\n");
         yPtr += stride_y_elements;
      }
      zPtr += stride_z_elements;
   }

   return;
}

template void AUDIOLIB_debugPrintMatrix3D_helper<int8_t>(int8_t *matrix, const AUDIOLIB_bufParams3D_t *params);
template void AUDIOLIB_debugPrintMatrix3D_helper<int16_t>(int16_t *matrix, const AUDIOLIB_bufParams3D_t *params);
template void AUDIOLIB_debugPrintMatrix3D_helper<int32_t>(int32_t *matrix, const AUDIOLIB_bufParams3D_t *params);

template <typename dataType>
void AUDIOLIB_debugPrintMatrix3D_helperU(dataType *matrix, const AUDIOLIB_bufParams3D_t *params)
{
   uint32_t  x, y, z;
   dataType *xPtr;
   dataType *yPtr;
   dataType *zPtr = matrix;

   // stride_y is stored in bytes, but easier to use in elements
   uint32_t stride_y_elements = params->stride_y / AUDIOLIB_sizeof(params->data_type);
   uint32_t stride_z_elements = params->stride_z / AUDIOLIB_sizeof(params->data_type);

   for (z = 0; z < params->dim_z; z++) {
      AUDIOLIB_PRINTF("%s", "\n");
      yPtr = zPtr;
      for (y = 0; y < params->dim_y; y++) {
         xPtr = yPtr;
         AUDIOLIB_PRINTF("%p |", xPtr);
         for (x = 0; x < params->dim_x; x++) {
            AUDIOLIB_PRINTF("%3u ", *(xPtr));
            xPtr++;
         }
         AUDIOLIB_PRINTF("%s", "|\n");
         yPtr += stride_y_elements;
      }
      zPtr += stride_z_elements;
   }

   return;
}

template void AUDIOLIB_debugPrintMatrix3D_helper<uint8_t>(uint8_t *matrix, const AUDIOLIB_bufParams3D_t *params);
template void AUDIOLIB_debugPrintMatrix3D_helper<uint16_t>(uint16_t *matrix, const AUDIOLIB_bufParams3D_t *params);

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

void AUDIOLIB_debugPrintMatrix3D(void *matrix, const AUDIOLIB_bufParams3D_t *params)
{
   switch (params->data_type) {
   case AUDIOLIB_INT32:
      AUDIOLIB_debugPrintMatrix3D_helper<int32_t>((int32_t *) matrix, params);
      break;
   case AUDIOLIB_INT16:
      AUDIOLIB_debugPrintMatrix3D_helper<int16_t>((int16_t *) matrix, params);
      break;
   case AUDIOLIB_UINT16:
      AUDIOLIB_debugPrintMatrix3D_helperU<uint16_t>((uint16_t *) matrix, params);
      break;
   case AUDIOLIB_INT8:
      AUDIOLIB_debugPrintMatrix3D_helper<int8_t>((int8_t *) matrix, params);
      break;
   case AUDIOLIB_UINT8:
      AUDIOLIB_debugPrintMatrix3D_helperU<uint8_t>((uint8_t *) matrix, params);
      break;
   default:
      AUDIOLIB_PRINTF("\nERROR: Unrecognized data type in %s.\n", __FUNCTION__);
   }

   return;
}
#ifdef __cplusplus
}
#endif /* __cplusplus */
