/* ************************************************************************
 * Copyright 2020-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "handle.hpp"

template <typename TScal, typename TConstPtr, typename TPtr>
rocblas_status rocblas_symm_arg_check(rocblas_handle handle,
                                      rocblas_side   side,
                                      rocblas_fill   uplo,
                                      rocblas_int    m,
                                      rocblas_int    n,
                                      TScal          alpha,
                                      TConstPtr      AP,
                                      rocblas_stride offsetA,
                                      rocblas_int    lda,
                                      rocblas_stride strideA,
                                      TConstPtr      BP,
                                      rocblas_stride offsetB,
                                      rocblas_int    ldb,
                                      rocblas_stride strideB,
                                      TScal          beta,
                                      const TPtr     CP,
                                      rocblas_stride offsetC,
                                      rocblas_int    ldc,
                                      rocblas_stride strideC,
                                      rocblas_int    batch_count);

template <bool HERM, typename TScal, typename TConstPtr, typename TPtr>
ROCBLAS_INTERNAL_EXPORT_NOINLINE rocblas_status
    rocblas_internal_symm_template(rocblas_handle handle,
                                   rocblas_side   side,
                                   rocblas_fill   uplo,
                                   rocblas_int    m,
                                   rocblas_int    n,
                                   TScal          alpha,
                                   TConstPtr      AP,
                                   rocblas_stride offsetA,
                                   rocblas_int    lda,
                                   rocblas_stride strideA,
                                   TConstPtr      BP,
                                   rocblas_stride offsetB,
                                   rocblas_int    ldb,
                                   rocblas_stride strideB,
                                   TScal          beta,
                                   TPtr           CP,
                                   rocblas_stride offsetC,
                                   rocblas_int    ldc,
                                   rocblas_stride strideC,
                                   rocblas_int    batch_count);
