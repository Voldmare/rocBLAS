/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "logging.hpp"
#include "rocblas_spr2.hpp"
#include "utility.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_spr2_strided_batched_name[] = "unknown";
    template <>
    constexpr char rocblas_spr2_strided_batched_name<float>[] = "rocblas_sspr2_strided_batched";
    template <>
    constexpr char rocblas_spr2_strided_batched_name<double>[] = "rocblas_dspr2_strided_batched";

    template <typename T>
    rocblas_status rocblas_spr2_strided_batched_impl(rocblas_handle handle,
                                                     rocblas_fill   uplo,
                                                     rocblas_int    n,
                                                     const T*       alpha,
                                                     const T*       x,
                                                     rocblas_int    incx,
                                                     rocblas_stride stride_x,
                                                     const T*       y,
                                                     rocblas_int    incy,
                                                     rocblas_stride stride_y,
                                                     T*             AP,
                                                     rocblas_stride strideA,
                                                     rocblas_int    batch_count)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode     = handle->layer_mode;
        auto check_numerics = handle->check_numerics;
        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto uplo_letter = rocblas_fill_letter(uplo);

            if(layer_mode & rocblas_layer_mode_log_trace)
                log_trace(handle,
                          rocblas_spr2_strided_batched_name<T>,
                          uplo,
                          n,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          x,
                          incx,
                          stride_x,
                          y,
                          incy,
                          stride_y,
                          AP,
                          strideA,
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          "./rocblas-bench -f spr2_strided_batched -r",
                          rocblas_precision_string<T>,
                          "--uplo",
                          uplo_letter,
                          "-n",
                          n,
                          LOG_BENCH_SCALAR_VALUE(handle, alpha),
                          "--incx",
                          incx,
                          "--incy",
                          incy,
                          "--stride_x",
                          stride_x,
                          "--stride_y",
                          stride_y,
                          "--stride_a",
                          strideA,
                          "--batch_count",
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_spr2_strided_batched_name<T>,
                            "uplo",
                            uplo_letter,
                            "N",
                            n,
                            "incx",
                            incx,
                            "incy",
                            incy,
                            "stride_x",
                            stride_x,
                            "stride_y",
                            stride_y,
                            "stride_a",
                            strideA,
                            "batch_count",
                            batch_count);
        }

        if(uplo != rocblas_fill_lower && uplo != rocblas_fill_upper)
            return rocblas_status_invalid_value;
        if(n < 0 || !incx || !incy || batch_count < 0)
            return rocblas_status_invalid_size;
        if(!n || !batch_count)
            return rocblas_status_success;
        if(!x || !y || !AP || !alpha)
            return rocblas_status_invalid_pointer;

        static constexpr rocblas_int offset_x = 0, offset_y = 0, offset_A = 0;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status spr2_check_numerics_status
                = rocblas_spr2_check_numerics(rocblas_spr2_strided_batched_name<T>,
                                              handle,
                                              n,
                                              AP,
                                              offset_A,
                                              strideA,
                                              x,
                                              offset_x,
                                              incx,
                                              stride_x,
                                              y,
                                              offset_y,
                                              incy,
                                              stride_y,
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(spr2_check_numerics_status != rocblas_status_success)
                return spr2_check_numerics_status;
        }

        rocblas_status status = rocblas_spr2_template(handle,
                                                      uplo,
                                                      n,
                                                      alpha,
                                                      x,
                                                      offset_x,
                                                      incx,
                                                      stride_x,
                                                      y,
                                                      offset_y,
                                                      incy,
                                                      stride_y,
                                                      AP,
                                                      offset_A,
                                                      strideA,
                                                      batch_count);
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status spr2_check_numerics_status
                = rocblas_spr2_check_numerics(rocblas_spr2_strided_batched_name<T>,
                                              handle,
                                              n,
                                              AP,
                                              offset_A,
                                              strideA,
                                              x,
                                              offset_x,
                                              incx,
                                              stride_x,
                                              y,
                                              offset_y,
                                              incy,
                                              stride_y,
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(spr2_check_numerics_status != rocblas_status_success)
                return spr2_check_numerics_status;
        }
        return status;
    }
}

/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

rocblas_status rocblas_sspr2_strided_batched(rocblas_handle handle,
                                             rocblas_fill   uplo,
                                             rocblas_int    n,
                                             const float*   alpha,
                                             const float*   x,
                                             rocblas_int    incx,
                                             rocblas_stride stride_x,
                                             const float*   y,
                                             rocblas_int    incy,
                                             rocblas_stride stride_y,
                                             float*         AP,
                                             rocblas_stride strideA,
                                             rocblas_int    batch_count)
try
{
    return rocblas_spr2_strided_batched_impl(
        handle, uplo, n, alpha, x, incx, stride_x, y, incy, stride_y, AP, strideA, batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

rocblas_status rocblas_dspr2_strided_batched(rocblas_handle handle,
                                             rocblas_fill   uplo,
                                             rocblas_int    n,
                                             const double*  alpha,
                                             const double*  x,
                                             rocblas_int    incx,
                                             rocblas_stride stride_x,
                                             const double*  y,
                                             rocblas_int    incy,
                                             rocblas_stride stride_y,
                                             double*        AP,
                                             rocblas_stride strideA,
                                             rocblas_int    batch_count)
try
{
    return rocblas_spr2_strided_batched_impl(
        handle, uplo, n, alpha, x, incx, stride_x, y, incy, stride_y, AP, strideA, batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

} // extern "C"
