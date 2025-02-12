/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "rocblas_her2.hpp"
#include "logging.hpp"
#include "utility.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_her2_name[] = "unknown";
    template <>
    constexpr char rocblas_her2_name<rocblas_float_complex>[] = "rocblas_cher2";
    template <>
    constexpr char rocblas_her2_name<rocblas_double_complex>[] = "rocblas_zher2";

    template <typename T>
    rocblas_status rocblas_her2_impl(rocblas_handle handle,
                                     rocblas_fill   uplo,
                                     rocblas_int    n,
                                     const T*       alpha,
                                     const T*       x,
                                     rocblas_int    incx,
                                     const T*       y,
                                     rocblas_int    incy,
                                     T*             A,
                                     rocblas_int    lda)
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
                          rocblas_her2_name<T>,
                          uplo,
                          n,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          x,
                          incx,
                          y,
                          incy,
                          A,
                          lda);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          "./rocblas-bench -f her2 -r",
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
                          "--lda",
                          lda);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_her2_name<T>,
                            "uplo",
                            uplo_letter,
                            "N",
                            n,
                            "incx",
                            incx,
                            "incy",
                            incy,
                            "lda",
                            lda);
        }

        if(uplo != rocblas_fill_lower && uplo != rocblas_fill_upper)
            return rocblas_status_invalid_value;
        if(n < 0 || !incx || !incy || lda < n || lda < 1)
            return rocblas_status_invalid_size;
        if(!n)
            return rocblas_status_success;
        if(!x || !y || !A || !alpha)
            return rocblas_status_invalid_pointer;

        static constexpr rocblas_int    offset_x = 0, offset_y = 0, offset_A = 0, batch_count = 1;
        static constexpr rocblas_stride stride_x = 0, stride_y = 0, stride_A = 0;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status her2_check_numerics_status
                = rocblas_her2_check_numerics(rocblas_her2_name<T>,
                                              handle,
                                              n,
                                              A,
                                              offset_A,
                                              lda,
                                              stride_A,
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
            if(her2_check_numerics_status != rocblas_status_success)
                return her2_check_numerics_status;
        }

        rocblas_status status = rocblas_internal_her2_template(handle,
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
                                                               A,
                                                               lda,
                                                               offset_A,
                                                               stride_A,
                                                               batch_count);
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status her2_check_numerics_status
                = rocblas_her2_check_numerics(rocblas_her2_name<T>,
                                              handle,
                                              n,
                                              A,
                                              offset_A,
                                              lda,
                                              stride_A,
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
            if(her2_check_numerics_status != rocblas_status_success)
                return her2_check_numerics_status;
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

rocblas_status rocblas_cher2(rocblas_handle               handle,
                             rocblas_fill                 uplo,
                             rocblas_int                  n,
                             const rocblas_float_complex* alpha,
                             const rocblas_float_complex* x,
                             rocblas_int                  incx,
                             const rocblas_float_complex* y,
                             rocblas_int                  incy,
                             rocblas_float_complex*       A,
                             rocblas_int                  lda)
try
{
    return rocblas_her2_impl(handle, uplo, n, alpha, x, incx, y, incy, A, lda);
}
catch(...)
{
    return exception_to_rocblas_status();
}

rocblas_status rocblas_zher2(rocblas_handle                handle,
                             rocblas_fill                  uplo,
                             rocblas_int                   n,
                             const rocblas_double_complex* alpha,
                             const rocblas_double_complex* x,
                             rocblas_int                   incx,
                             const rocblas_double_complex* y,
                             rocblas_int                   incy,
                             rocblas_double_complex*       A,
                             rocblas_int                   lda)
try
{
    return rocblas_her2_impl(handle, uplo, n, alpha, x, incx, y, incy, A, lda);
}
catch(...)
{
    return exception_to_rocblas_status();
}

} // extern "C"
