/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "logging.hpp"
#include "rocblas_hbmv.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_hbmv_name[] = "unknown";
    template <>
    constexpr char rocblas_hbmv_name<rocblas_float_complex>[] = "rocblas_chbmv_strided_batched";
    template <>
    constexpr char rocblas_hbmv_name<rocblas_double_complex>[] = "rocblas_zhbmv_strided_batched";

    template <typename T>
    rocblas_status rocblas_hbmv_strided_batched_impl(rocblas_handle handle,
                                                     rocblas_fill   uplo,
                                                     rocblas_int    n,
                                                     rocblas_int    k,
                                                     const T*       alpha,
                                                     const T*       A,
                                                     rocblas_int    lda,
                                                     rocblas_stride stride_A,
                                                     const T*       x,
                                                     rocblas_int    incx,
                                                     rocblas_stride stride_x,
                                                     const T*       beta,
                                                     T*             y,
                                                     rocblas_int    incy,
                                                     rocblas_stride stride_y,
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
                          rocblas_hbmv_name<T>,
                          uplo,
                          n,
                          k,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          A,
                          lda,
                          stride_A,
                          x,
                          incx,
                          stride_x,
                          LOG_TRACE_SCALAR_VALUE(handle, beta),
                          y,
                          incy,
                          stride_y,
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          "./rocblas-bench -f hbmv_strided_batched -r",
                          rocblas_precision_string<T>,
                          "--uplo",
                          uplo_letter,
                          "-n",
                          n,
                          "-k",
                          k,
                          LOG_BENCH_SCALAR_VALUE(handle, alpha),
                          "--lda",
                          lda,
                          "--stride_a",
                          stride_A,
                          "--incx",
                          incx,
                          "--stride_x",
                          stride_x,
                          LOG_BENCH_SCALAR_VALUE(handle, beta),
                          "--incy",
                          incy,
                          "--stride_y",
                          stride_y,
                          "--batch_count",
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_hbmv_name<T>,
                            "uplo",
                            uplo_letter,
                            "N",
                            n,
                            "k",
                            k,
                            "lda",
                            lda,
                            "stride_a",
                            stride_A,
                            "incx",
                            incx,
                            "stride_x",
                            stride_x,
                            "incy",
                            incy,
                            "stride_y",
                            stride_y,
                            "batch_count",
                            batch_count);
        }

        if(n < 0 || k < 0 || lda <= k || !incx || !incy || batch_count < 0)
            return rocblas_status_invalid_size;

        if(!n || !batch_count)
            return rocblas_status_success;

        if(!alpha || !beta)
            return rocblas_status_invalid_pointer;

        if(handle->pointer_mode == rocblas_pointer_mode_host && !*alpha)
        {
            if(*beta == 1)
                return rocblas_status_success;
        }
        else if(!A || !x)
            return rocblas_status_invalid_pointer;

        if(!y)
            return rocblas_status_invalid_pointer;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status hbmv_check_numerics_status
                = rocblas_hbmv_check_numerics(rocblas_hbmv_name<T>,
                                              handle,
                                              n,
                                              k,
                                              A,
                                              0,
                                              lda,
                                              stride_A,
                                              x,
                                              0,
                                              incx,
                                              stride_x,
                                              y,
                                              0,
                                              incy,
                                              stride_y,
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(hbmv_check_numerics_status != rocblas_status_success)
                return hbmv_check_numerics_status;
        }

        rocblas_status status = rocblas_hbmv_template(handle,
                                                      uplo,
                                                      n,
                                                      k,
                                                      alpha,
                                                      A,
                                                      0,
                                                      lda,
                                                      stride_A,
                                                      x,
                                                      0,
                                                      incx,
                                                      stride_x,
                                                      beta,
                                                      y,
                                                      0,
                                                      incy,
                                                      stride_y,
                                                      batch_count);
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status hbmv_check_numerics_status
                = rocblas_hbmv_check_numerics(rocblas_hbmv_name<T>,
                                              handle,
                                              n,
                                              k,
                                              A,
                                              0,
                                              lda,
                                              stride_A,
                                              x,
                                              0,
                                              incx,
                                              stride_x,
                                              y,
                                              0,
                                              incy,
                                              stride_y,
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(hbmv_check_numerics_status != rocblas_status_success)
                return hbmv_check_numerics_status;
        }
        return status;
    }

} // namespace

/*
* ===========================================================================
*    C wrapper
* ===========================================================================
*/

extern "C" {

rocblas_status rocblas_chbmv_strided_batched(rocblas_handle               handle,
                                             rocblas_fill                 uplo,
                                             rocblas_int                  n,
                                             rocblas_int                  k,
                                             const rocblas_float_complex* alpha,
                                             const rocblas_float_complex* A,
                                             rocblas_int                  lda,
                                             rocblas_stride               stride_A,
                                             const rocblas_float_complex* x,
                                             rocblas_int                  incx,
                                             rocblas_stride               stride_x,
                                             const rocblas_float_complex* beta,
                                             rocblas_float_complex*       y,
                                             rocblas_int                  incy,
                                             rocblas_stride               stride_y,
                                             rocblas_int                  batch_count)
try
{
    return rocblas_hbmv_strided_batched_impl(handle,
                                             uplo,
                                             n,
                                             k,
                                             alpha,
                                             A,
                                             lda,
                                             stride_A,
                                             x,
                                             incx,
                                             stride_x,
                                             beta,
                                             y,
                                             incy,
                                             stride_y,
                                             batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

rocblas_status rocblas_zhbmv_strided_batched(rocblas_handle                handle,
                                             rocblas_fill                  uplo,
                                             rocblas_int                   n,
                                             rocblas_int                   k,
                                             const rocblas_double_complex* alpha,
                                             const rocblas_double_complex* A,
                                             rocblas_int                   lda,
                                             rocblas_stride                stride_A,
                                             const rocblas_double_complex* x,
                                             rocblas_int                   incx,
                                             rocblas_stride                stride_x,
                                             const rocblas_double_complex* beta,
                                             rocblas_double_complex*       y,
                                             rocblas_int                   incy,
                                             rocblas_stride                stride_y,
                                             rocblas_int                   batch_count)
try
{
    return rocblas_hbmv_strided_batched_impl(handle,
                                             uplo,
                                             n,
                                             k,
                                             alpha,
                                             A,
                                             lda,
                                             stride_A,
                                             x,
                                             incx,
                                             stride_x,
                                             beta,
                                             y,
                                             incy,
                                             stride_y,
                                             batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

} // extern "C"
