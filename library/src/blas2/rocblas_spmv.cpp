/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#include "rocblas_spmv.hpp"
#include "logging.hpp"
#include "utility.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_spmv_name[] = "unknown";
    template <>
    constexpr char rocblas_spmv_name<float>[] = "rocblas_sspmv";
    template <>
    constexpr char rocblas_spmv_name<double>[] = "rocblas_dspmv";

    template <typename T, typename U, typename V, typename W>
    rocblas_status rocblas_spmv_impl(rocblas_handle handle,
                                     rocblas_fill   uplo,
                                     rocblas_int    n,
                                     const V*       alpha,
                                     const U*       A,
                                     const U*       x,
                                     rocblas_int    incx,
                                     const V*       beta,
                                     W*             y,
                                     rocblas_int    incy)
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
                          rocblas_spmv_name<T>,
                          uplo,
                          n,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          A,
                          x,
                          incx,
                          LOG_TRACE_SCALAR_VALUE(handle, beta),
                          y,
                          incy);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          "./rocblas-bench -f spmv -r",
                          rocblas_precision_string<T>,
                          "--uplo",
                          uplo_letter,
                          "-n",
                          n,
                          LOG_BENCH_SCALAR_VALUE(handle, alpha),
                          "--incx",
                          incx,
                          LOG_BENCH_SCALAR_VALUE(handle, beta),
                          "--incy",
                          incy);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_spmv_name<T>,
                            "uplo",
                            uplo_letter,
                            "N",
                            n,
                            "incx",
                            incx,
                            "incy",
                            incy);
        }

        rocblas_status arg_status = rocblas_spmv_arg_check<T>(
            handle, uplo, n, alpha, 0, A, 0, 0, x, 0, incx, 0, beta, 0, y, 0, incy, 0, 1);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status spmv_check_numerics_status
                = rocblas_spmv_check_numerics(rocblas_spmv_name<T>,
                                              handle,
                                              n,
                                              A,
                                              0,
                                              0,
                                              x,
                                              0,
                                              incx,
                                              0,
                                              y,
                                              0,
                                              incy,
                                              0,
                                              1,
                                              check_numerics,
                                              is_input);
            if(spmv_check_numerics_status != rocblas_status_success)
                return spmv_check_numerics_status;
        }

        rocblas_status status = rocblas_spmv_template<T>(
            handle, uplo, n, alpha, 0, A, 0, 0, x, 0, incx, 0, beta, 0, y, 0, incy, 0, 1);
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status spmv_check_numerics_status
                = rocblas_spmv_check_numerics(rocblas_spmv_name<T>,
                                              handle,
                                              n,
                                              A,
                                              0,
                                              0,
                                              x,
                                              0,
                                              incx,
                                              0,
                                              y,
                                              0,
                                              incy,
                                              0,
                                              1,
                                              check_numerics,
                                              is_input);
            if(spmv_check_numerics_status != rocblas_status_success)
                return spmv_check_numerics_status;
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

#ifdef IMPL
#error IMPL ALREADY DEFINED
#endif

#define IMPL(routine_name_, T_)                                                          \
    rocblas_status routine_name_(rocblas_handle handle,                                  \
                                 rocblas_fill   uplo,                                    \
                                 rocblas_int    n,                                       \
                                 const T_*      alpha,                                   \
                                 const T_*      A,                                       \
                                 const T_*      x,                                       \
                                 rocblas_int    incx,                                    \
                                 const T_*      beta,                                    \
                                 T_*            y,                                       \
                                 rocblas_int    incy)                                    \
    try                                                                                  \
    {                                                                                    \
        return rocblas_spmv_impl<T_>(handle, uplo, n, alpha, A, x, incx, beta, y, incy); \
    }                                                                                    \
    catch(...)                                                                           \
    {                                                                                    \
        return exception_to_rocblas_status();                                            \
    }

IMPL(rocblas_sspmv, float);
IMPL(rocblas_dspmv, double);

#undef IMPL

} // extern "C"
