/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "rocblas_syr.hpp"
#include "logging.hpp"
#include "utility.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_syr_name[] = "unknown";
    template <>
    constexpr char rocblas_syr_name<float>[] = "rocblas_ssyr";
    template <>
    constexpr char rocblas_syr_name<double>[] = "rocblas_dsyr";
    template <>
    constexpr char rocblas_syr_name<rocblas_float_complex>[] = "rocblas_csyr";
    template <>
    constexpr char rocblas_syr_name<rocblas_double_complex>[] = "rocblas_zsyr";

    template <typename T>
    rocblas_status rocblas_syr_impl(rocblas_handle handle,
                                    rocblas_fill   uplo,
                                    rocblas_int    n,
                                    const T*       alpha,
                                    const T*       x,
                                    rocblas_int    incx,
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
                          rocblas_syr_name<T>,
                          uplo,
                          n,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          x,
                          incx,
                          A,
                          lda);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          "./rocblas-bench -f syr -r",
                          rocblas_precision_string<T>,
                          "--uplo",
                          uplo_letter,
                          "-n",
                          n,
                          LOG_BENCH_SCALAR_VALUE(handle, alpha),
                          "--incx",
                          incx,
                          "--lda",
                          lda);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_syr_name<T>,
                            "uplo",
                            uplo_letter,
                            "N",
                            n,
                            "incx",
                            incx,
                            "lda",
                            lda);
        }

        rocblas_status arg_status
            = rocblas_syr_arg_check<T>(uplo, n, alpha, 0, x, 0, incx, 0, A, 0, lda, 0, 1);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status syr_check_numerics_status
                = rocblas_syr_check_numerics(rocblas_syr_name<T>,
                                             handle,
                                             n,
                                             A,
                                             0,
                                             lda,
                                             0,
                                             x,
                                             0,
                                             incx,
                                             0,
                                             1,
                                             check_numerics,
                                             is_input);
            if(syr_check_numerics_status != rocblas_status_success)
                return syr_check_numerics_status;
        }
        rocblas_status status = rocblas_internal_syr_template<T>(
            handle, uplo, n, alpha, 0, x, 0, incx, 0, A, 0, lda, 0, 1);
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status syr_check_numerics_status
                = rocblas_syr_check_numerics(rocblas_syr_name<T>,
                                             handle,
                                             n,
                                             A,
                                             0,
                                             lda,
                                             0,
                                             x,
                                             0,
                                             incx,
                                             0,
                                             1,
                                             check_numerics,
                                             is_input);
            if(syr_check_numerics_status != rocblas_status_success)
                return syr_check_numerics_status;
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

#ifdef IMPL
#error IMPL ALREADY DEFINED
#endif

#define IMPL(routine_name_, T_)                                           \
    rocblas_status routine_name_(rocblas_handle handle,                   \
                                 rocblas_fill   uplo,                     \
                                 rocblas_int    n,                        \
                                 const T_*      alpha,                    \
                                 const T_*      x,                        \
                                 rocblas_int    incx,                     \
                                 T_*            A,                        \
                                 rocblas_int    lda)                      \
    try                                                                   \
    {                                                                     \
        return rocblas_syr_impl(handle, uplo, n, alpha, x, incx, A, lda); \
    }                                                                     \
    catch(...)                                                            \
    {                                                                     \
        return exception_to_rocblas_status();                             \
    }

IMPL(rocblas_ssyr, float);
IMPL(rocblas_dsyr, double);
IMPL(rocblas_csyr, rocblas_float_complex);
IMPL(rocblas_zsyr, rocblas_double_complex);

#undef IMPL

} // extern "C"
