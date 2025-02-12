/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "check_numerics_vector.hpp"
#include "handle.hpp"

template <typename T, typename U>
rocblas_status rocblas_internal_trsv_check_numerics(const char*       function_name,
                                                    rocblas_handle    handle,
                                                    rocblas_int       m,
                                                    T                 A,
                                                    rocblas_stride    offset_a,
                                                    rocblas_int       lda,
                                                    rocblas_stride    stride_a,
                                                    U                 x,
                                                    rocblas_stride    offset_x,
                                                    rocblas_int       inc_x,
                                                    rocblas_stride    stride_x,
                                                    rocblas_int       batch_count,
                                                    const rocblas_int check_numerics,
                                                    bool              is_input);

template <rocblas_int DIM_X, typename T, typename ATYPE, typename XTYPE>
ROCBLAS_INTERNAL_EXPORT_NOINLINE rocblas_status
    rocblas_internal_trsv_substitution_template(rocblas_handle    handle,
                                                rocblas_fill      uplo,
                                                rocblas_operation transA,
                                                rocblas_diagonal  diag,
                                                rocblas_int       m,
                                                ATYPE             dA,
                                                rocblas_stride    offset_A,
                                                rocblas_int       lda,
                                                rocblas_stride    stride_A,
                                                T const*          alpha,
                                                XTYPE             dx,
                                                rocblas_stride    offset_x,
                                                rocblas_int       incx,
                                                rocblas_stride    stride_x,
                                                rocblas_int       batch_count,
                                                rocblas_int*      w_completed_sec);
