/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "handle.hpp"
#include "logging.hpp"
#include "rocblas.h"
#include "rocblas_trmv.hpp"
#include "utility.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_trmv_batched_name[] = "unknown";
    template <>
    constexpr char rocblas_trmv_batched_name<float>[] = "rocblas_strmv_batched";
    template <>
    constexpr char rocblas_trmv_batched_name<double>[] = "rocblas_dtrmv_batched";
    template <>
    constexpr char rocblas_trmv_batched_name<rocblas_float_complex>[] = "rocblas_ctrmv_batched";
    template <>
    constexpr char rocblas_trmv_batched_name<rocblas_double_complex>[] = "rocblas_ztrmv_batched";

    template <typename T>
    rocblas_status rocblas_trmv_batched_impl(rocblas_handle    handle,
                                             rocblas_fill      uplo,
                                             rocblas_operation transa,
                                             rocblas_diagonal  diag,
                                             rocblas_int       m,
                                             const T* const*   a,
                                             rocblas_int       lda,
                                             T* const*         x,
                                             rocblas_int       incx,
                                             rocblas_int       batch_count)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        if(!handle->is_device_memory_size_query())
        {
            auto layer_mode = handle->layer_mode;
            if(layer_mode
               & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
                  | rocblas_layer_mode_log_profile))
            {
                auto uplo_letter   = rocblas_fill_letter(uplo);
                auto transa_letter = rocblas_transpose_letter(transa);
                auto diag_letter   = rocblas_diag_letter(diag);
                if(layer_mode & rocblas_layer_mode_log_trace)
                {
                    log_trace(handle,
                              rocblas_trmv_batched_name<T>,
                              uplo,
                              transa,
                              diag,
                              m,
                              a,
                              lda,
                              x,
                              incx,
                              batch_count);
                }

                if(layer_mode & rocblas_layer_mode_log_bench)
                {
                    log_bench(handle,
                              "./rocblas-bench",
                              "-f",
                              "trmv_batched",
                              "-r",
                              rocblas_precision_string<T>,
                              "--uplo",
                              uplo_letter,
                              "--transposeA",
                              transa_letter,
                              "--diag",
                              diag_letter,
                              "-m",
                              m,
                              "--lda",
                              lda,
                              "--incx",
                              incx,
                              "--batch_count",
                              batch_count);
                }

                if(layer_mode & rocblas_layer_mode_log_profile)
                {
                    log_profile(handle,
                                rocblas_trmv_batched_name<T>,
                                "uplo",
                                uplo_letter,
                                "transA",
                                transa_letter,
                                "diag",
                                diag_letter,
                                "M",
                                m,
                                "lda",
                                lda,
                                "incx",
                                incx,
                                "batch_count",
                                batch_count);
                }
            }
        }

        if(uplo != rocblas_fill_lower && uplo != rocblas_fill_upper)
            return rocblas_status_invalid_value;

        if(m < 0 || lda < m || lda < 1 || !incx || batch_count < 0)
            return rocblas_status_invalid_size;

        if(!m || !batch_count)
        {
            RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);
            return rocblas_status_success;
        }

        size_t dev_bytes = m * batch_count * sizeof(T);
        if(handle->is_device_memory_size_query())
            return handle->set_optimal_device_memory_size(dev_bytes);

        if(!a || !x)
            return rocblas_status_invalid_pointer;

        auto workspace = handle->device_malloc(dev_bytes);
        if(!workspace)
            return rocblas_status_memory_error;

        rocblas_stride stride_w = m;

        auto check_numerics = handle->check_numerics;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status trmv_check_numerics_status
                = rocblas_trmv_check_numerics(rocblas_trmv_batched_name<T>,
                                              handle,
                                              m,
                                              a,
                                              0,
                                              lda,
                                              0,
                                              x,
                                              0,
                                              incx,
                                              0,
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(trmv_check_numerics_status != rocblas_status_success)
                return trmv_check_numerics_status;
        }

        constexpr rocblas_int    offset_a = 0, offset_x = 0;
        constexpr rocblas_stride stride_a = 0, stride_x = 0;
        rocblas_status           status = rocblas_internal_trmv_template(handle,
                                                               uplo,
                                                               transa,
                                                               diag,
                                                               m,
                                                               a,
                                                               offset_a,
                                                               lda,
                                                               stride_a,
                                                               x,
                                                               offset_x,
                                                               incx,
                                                               stride_x,
                                                               (T*)workspace,
                                                               stride_w,
                                                               batch_count);

        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status trmv_check_numerics_status
                = rocblas_trmv_check_numerics(rocblas_trmv_batched_name<T>,
                                              handle,
                                              m,
                                              a,
                                              0,
                                              lda,
                                              0,
                                              x,
                                              0,
                                              incx,
                                              0,
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(trmv_check_numerics_status != rocblas_status_success)
                return trmv_check_numerics_status;
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

#define IMPL(routine_name_, T_)                                           \
    rocblas_status routine_name_(rocblas_handle    handle,                \
                                 rocblas_fill      uplo,                  \
                                 rocblas_operation transa,                \
                                 rocblas_diagonal  diag,                  \
                                 rocblas_int       m,                     \
                                 const T_* const*  a,                     \
                                 rocblas_int       lda,                   \
                                 T_* const*        x,                     \
                                 rocblas_int       incx,                  \
                                 rocblas_int       batch_count)           \
    try                                                                   \
    {                                                                     \
        return rocblas_trmv_batched_impl(                                 \
            handle, uplo, transa, diag, m, a, lda, x, incx, batch_count); \
    }                                                                     \
    catch(...)                                                            \
    {                                                                     \
        return exception_to_rocblas_status();                             \
    }

IMPL(rocblas_strmv_batched, float);
IMPL(rocblas_dtrmv_batched, double);
IMPL(rocblas_ctrmv_batched, rocblas_float_complex);
IMPL(rocblas_ztrmv_batched, rocblas_double_complex);

#undef IMPL

} // extern "C"
