// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef C71_AUDIOLIB_INLINES_H
#define C71_AUDIOLIB_INLINES_H

#include "../AUDIOLIB_types.h"
#if defined(AUDIOLIB_DEBUGPRINT)
#include "AUDIOLIB_debug.h"
#endif
#include <float.h>  // for max float, double values
#include <limits.h> // for min, max integer values

/*******************************************************************************
 *
 * Inlined functions
 *
 ******************************************************************************/

/******************************************************************************/
#include "c7x_inlines.h"

#include "AUDIOLIB_UTIL_SE0AloadSE1BloadComputeC.h"
#include "AUDIOLIB_UTIL_SE0AloadSE1BloadComputeCSA0Cstore.h"
#include "AUDIOLIB_UTIL_SE1Bload.h"
#include "AUDIOLIB_UTIL_SE1BloadSA0Cstore.h"

#include "AUDIOLIB_UTIL_Q_SE0AloadSE1BloadComputeC.h"
#include "AUDIOLIB_UTIL_Q_SE0AloadSE1BloadComputeCSA0CstoreUnroll3.h"
#include "AUDIOLIB_UTIL_Q_SE1Bload.h"

#endif
