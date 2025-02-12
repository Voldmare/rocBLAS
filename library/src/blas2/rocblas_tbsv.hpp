/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "../blas1/rocblas_copy.hpp"
#include "check_numerics_vector.hpp"

template <rocblas_int BLOCK, typename TConstPtr, typename TPtr>
rocblas_status rocblas_tbsv_template(rocblas_handle    handle,
                                     rocblas_fill      uplo,
                                     rocblas_operation transA,
                                     rocblas_diagonal  diag,
                                     rocblas_int       n,
                                     rocblas_int       k,
                                     TConstPtr         A,
                                     rocblas_int       offset_A,
                                     rocblas_int       lda,
                                     rocblas_stride    stride_A,
                                     TPtr              x,
                                     rocblas_int       offset_x,
                                     rocblas_int       incx,
                                     rocblas_stride    stride_x,
                                     rocblas_int       batch_count);

//TODO :-Add rocblas_check_numerics_tb_matrix_template for checking Matrix `A` which is a Triangular Band Matrix
template <typename T, typename U>
rocblas_status rocblas_tbsv_check_numerics(const char*    function_name,
                                           rocblas_handle handle,
                                           rocblas_int    n,
                                           T              A,
                                           rocblas_stride offset_a,
                                           rocblas_int    lda,
                                           rocblas_stride stride_a,
                                           U              x,
                                           rocblas_stride offset_x,
                                           rocblas_int    inc_x,
                                           rocblas_stride stride_x,
                                           rocblas_int    batch_count,
                                           const int      check_numerics,
                                           bool           is_input);
