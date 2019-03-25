//------------------------------------------------------------------------------------------------------------------------------
// Samuel Williams
// SWWilliams@lbl.gov
// Lawrence Berkeley National Lab
//------------------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

//------------------------------------------------------------------------------------------------------------------------------
#include "timer.h"
#include "defines.h"
#include "box.h"
#include "mg.h"
//------------------------------------------------------------------------------------------------------------------------------
#include "exchange_boundary.inc"
#include "lambda.inc"
#include "jacobi.inc"
//#include "operators.ompif/gsrb.inc"
//#include "operators.ompif/chebyshev.inc"
#include "apply_op.inc"
#include "residual.inc"
#include "restriction.inc"
#include "interpolation.inc"
#include "misc.inc"
#include "matmul.inc"
#include "problem1.inc"
//#include "operators.ompif/problem2.inc"
//------------------------------------------------------------------------------------------------------------------------------
static const char *exchange_boundary_OptimisticChoices = "#c11#c41111#c5111111#c6111111#c:1#c<2#c=2#c>2#c?22222222222222#c@2#cA111#cB111#cD3#cE=";
const char **KeepAlive_exchange_boundary_operators.ompif = &exchange_boundary_OptimisticChoices;
static const char *smooth_OptimisticChoices = "#c11#c41111111111111#c511111111111111111111111111111111111111111#c<2#c=222222222222222222#c>2#c?2222222222222222222222222222222222222222222#c@222222222222222222#cB11111111#cD3#c81#c911111";
const char **KeepAlive_smooth_operators.ompif = &smooth_OptimisticChoices;
static const char *residual_OptimisticChoices = "#c11#c41111#c51111111111111111111111#c<2#c=222222222222#c>2#c?2222222222222222222222222222222222#c@222222222222#cB1111111#cD3#c81#c9111";
const char **KeepAlive_residual_operators.ompif = &residual_OptimisticChoices;
static const char *residual_and_restriction_OptimisticChoices = "#c11#c411111111111111111111111111#c5111111111111111111111111111111111111111111111111111111111111111#c<2#c=2222222222222#c>2#c?22222222222222222222222222222222222222222#c@2222222222222#cB1111111#cD3#c6111#c81111#c91111";
const char **KeepAlive_residual_and_restriction_operators.ompif = &residual_and_restriction_OptimisticChoices;
static const char *DoBufferCopy_OptimisticChoices = "#c11#c:1#c<2#c>2#cE=";
const char **KeepAlive_DoBufferCopy_operators.ompif = &DoBufferCopy_OptimisticChoices;
static const char *CycleTime_OptimisticChoices = "#c71#cE=";
const char **KeepAlive_CycleTime_operators.ompif = &CycleTime_OptimisticChoices;
