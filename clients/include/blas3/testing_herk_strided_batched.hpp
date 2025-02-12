/* ************************************************************************
 * Copyright 2020-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "bytes.hpp"
#include "cblas_interface.hpp"
#include "flops.hpp"
#include "near.hpp"
#include "norm.hpp"
#include "rocblas.hpp"
#include "rocblas_datatype2string.hpp"
#include "rocblas_init.hpp"
#include "rocblas_math.hpp"
#include "rocblas_random.hpp"
#include "rocblas_test.hpp"
#include "rocblas_vector.hpp"
#include "unit.hpp"
#include "utility.hpp"

template <typename T>
void testing_herk_strided_batched_bad_arg(const Arguments& arg)
{
    auto rocblas_herk_strided_batched_fn = arg.fortran
                                               ? rocblas_herk_strided_batched<T, real_t<T>, true>
                                               : rocblas_herk_strided_batched<T, real_t<T>, false>;

    rocblas_local_handle    handle{arg};
    const rocblas_fill      uplo   = rocblas_fill_upper;
    const rocblas_operation transA = rocblas_operation_none;
    const rocblas_int       N      = 100;
    const rocblas_int       K      = 100;
    const rocblas_int       lda    = 100;
    const rocblas_int       ldc    = 100;
    using U                        = real_t<T>;
    const U        alpha           = 1.0;
    const U        beta            = 1.0;
    rocblas_stride strideA         = 1;
    rocblas_stride strideC         = 1;
    rocblas_int    batch_count     = 2;

    const size_t safe_size = 100;
    // allocate memory on device
    device_vector<T> dA(batch_count);
    device_vector<T> dC(batch_count);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dC.memcheck());

    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(nullptr,
                                                            uplo,
                                                            transA,
                                                            N,
                                                            K,
                                                            &alpha,
                                                            dA,
                                                            lda,
                                                            strideA,
                                                            &beta,
                                                            dC,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_invalid_handle);

    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                            rocblas_fill_full,
                                                            transA,
                                                            N,
                                                            K,
                                                            &alpha,
                                                            dA,
                                                            lda,
                                                            strideA,
                                                            &beta,
                                                            dC,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_invalid_value);

    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                            uplo,
                                                            rocblas_operation_transpose,
                                                            N,
                                                            K,
                                                            &alpha,
                                                            dA,
                                                            lda,
                                                            strideA,
                                                            &beta,
                                                            dC,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_invalid_value);

    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                            uplo,
                                                            transA,
                                                            N,
                                                            K,
                                                            nullptr,
                                                            dA,
                                                            lda,
                                                            strideA,
                                                            &beta,
                                                            dC,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                            uplo,
                                                            transA,
                                                            N,
                                                            K,
                                                            &alpha,
                                                            nullptr,
                                                            lda,
                                                            strideA,
                                                            &beta,
                                                            dC,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                            uplo,
                                                            transA,
                                                            N,
                                                            K,
                                                            &alpha,
                                                            dA,
                                                            lda,
                                                            strideA,
                                                            nullptr,
                                                            dC,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                            uplo,
                                                            transA,
                                                            N,
                                                            K,
                                                            &alpha,
                                                            dA,
                                                            lda,
                                                            strideA,
                                                            &beta,
                                                            nullptr,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_invalid_pointer);

    // quick return with invalid pointers
    EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                            uplo,
                                                            transA,
                                                            0,
                                                            K,
                                                            nullptr,
                                                            nullptr,
                                                            lda,
                                                            strideA,
                                                            nullptr,
                                                            nullptr,
                                                            ldc,
                                                            strideC,
                                                            batch_count),
                          rocblas_status_success);
}

template <typename T>
void testing_herk_strided_batched(const Arguments& arg)
{
    auto rocblas_herk_strided_batched_fn = arg.fortran
                                               ? rocblas_herk_strided_batched<T, real_t<T>, true>
                                               : rocblas_herk_strided_batched<T, real_t<T>, false>;

    rocblas_local_handle handle{arg};
    rocblas_fill         uplo   = char2rocblas_fill(arg.uplo);
    rocblas_operation    transA = char2rocblas_operation(arg.transA);
    rocblas_int          N      = arg.N;
    rocblas_int          K      = arg.K;
    rocblas_int          lda    = arg.lda;
    rocblas_int          ldc    = arg.ldc;
    using U                     = real_t<T>;
    U              alpha        = arg.get_alpha<U>();
    U              beta         = arg.get_beta<U>();
    rocblas_stride strideA      = arg.stride_a;
    rocblas_stride strideC      = arg.stride_c;
    rocblas_int    batch_count  = arg.batch_count;

    double gpu_time_used, cpu_time_used;
    double rocblas_error = 0.0;

    // Note: K==0 is not an early exit, since C still needs to be multiplied by beta
    bool invalid_size = N < 0 || K < 0 || ldc < N || (transA == rocblas_operation_none && lda < N)
                        || (transA != rocblas_operation_none && lda < K) || batch_count < 0;
    if(N == 0 || batch_count == 0 || invalid_size)
    {
        // ensure invalid sizes checked before pointer check
        EXPECT_ROCBLAS_STATUS((rocblas_herk_strided_batched_fn)(handle,
                                                                uplo,
                                                                transA,
                                                                N,
                                                                K,
                                                                nullptr,
                                                                nullptr,
                                                                lda,
                                                                strideA,
                                                                nullptr,
                                                                nullptr,
                                                                ldc,
                                                                strideC,
                                                                batch_count),
                              invalid_size ? rocblas_status_invalid_size : rocblas_status_success);

        return;
    }

    strideA = std::max(strideA,
                       rocblas_stride(size_t(lda) * (transA == rocblas_operation_none ? K : N)));
    strideC = std::max(strideC, rocblas_stride(size_t(ldc) * N));

    size_t size_A = strideA * batch_count;
    size_t size_C = strideC * batch_count;

    // allocate memory on device
    device_vector<T> dA(size_A);
    device_vector<T> dC(size_C);
    device_vector<U> d_alpha(1);
    device_vector<U> d_beta(1);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dC.memcheck());
    CHECK_DEVICE_ALLOCATION(d_alpha.memcheck());
    CHECK_DEVICE_ALLOCATION(d_beta.memcheck());

    // Naming: dX is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<U> h_alpha(1);
    host_vector<U> h_beta(1);
    host_vector<T> hA(size_A);
    host_vector<T> hC_1(size_C);
    host_vector<T> hC_2(size_C);
    host_vector<T> hC_gold(size_C);

    CHECK_HIP_ERROR(h_alpha.memcheck());
    CHECK_HIP_ERROR(h_beta.memcheck());
    CHECK_HIP_ERROR(hA.memcheck());
    CHECK_HIP_ERROR(hC_1.memcheck());
    CHECK_HIP_ERROR(hC_2.memcheck());
    CHECK_HIP_ERROR(hC_gold.memcheck());

    // Initial Data on CPU
    h_alpha[0] = alpha;
    h_beta[0]  = beta;

    // Initialize data on host memory
    rocblas_init_matrix(hA,
                        arg,
                        size_A,
                        1,
                        1,
                        0,
                        1,
                        rocblas_client_alpha_sets_nan,
                        rocblas_client_triangular_matrix,
                        true);
    rocblas_init_matrix(hC_1,
                        arg,
                        N,
                        N,
                        ldc,
                        strideC,
                        batch_count,
                        rocblas_client_beta_sets_nan,
                        rocblas_client_hermitian_matrix,
                        false,
                        true);

    hC_2    = hC_1;
    hC_gold = hC_1;

    // copy data from CPU to device
    CHECK_HIP_ERROR(dA.transfer_from(hA));

    if(arg.unit_check || arg.norm_check)
    {
        // host alpha/beta
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        CHECK_HIP_ERROR(dC.transfer_from(hC_1));

        CHECK_ROCBLAS_ERROR((rocblas_herk_strided_batched_fn)(handle,
                                                              uplo,
                                                              transA,
                                                              N,
                                                              K,
                                                              &h_alpha[0],
                                                              dA,
                                                              lda,
                                                              strideA,
                                                              &h_beta[0],
                                                              dC,
                                                              ldc,
                                                              strideC,
                                                              batch_count));

        // copy output from device to CPU
        CHECK_HIP_ERROR(hC_1.transfer_from(dC));

        // device alpha/beta
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_HIP_ERROR(dC.transfer_from(hC_2));
        CHECK_HIP_ERROR(d_alpha.transfer_from(h_alpha));
        CHECK_HIP_ERROR(d_beta.transfer_from(h_beta));

        CHECK_ROCBLAS_ERROR((rocblas_herk_strided_batched_fn)(handle,
                                                              uplo,
                                                              transA,
                                                              N,
                                                              K,
                                                              d_alpha,
                                                              dA,
                                                              lda,
                                                              strideA,
                                                              d_beta,
                                                              dC,
                                                              ldc,
                                                              strideC,
                                                              batch_count));

        // CPU BLAS
        if(arg.timing)
        {
            cpu_time_used = get_time_us_no_sync();
        }

        // cpu reference
        for(int i = 0; i < batch_count; i++)
        {
            cblas_herk<T>(uplo,
                          transA,
                          N,
                          K,
                          h_alpha[0],
                          hA + i * strideA,
                          lda,
                          h_beta[0],
                          hC_gold + i * strideC,
                          ldc);
        }

        if(arg.timing)
        {
            cpu_time_used = get_time_us_no_sync() - cpu_time_used;
        }

        // copy output from device to CPU
        CHECK_HIP_ERROR(hC_2.transfer_from(dC));

        if(arg.unit_check)
        {
            if(std::is_same<T, rocblas_float_complex>{}
               || std::is_same<T, rocblas_double_complex>{})
            {
                const double tol = K * sum_error_tolerance<T>;
                near_check_general<T>(N, N, ldc, strideC, hC_gold, hC_1, batch_count, tol);
                near_check_general<T>(N, N, ldc, strideC, hC_gold, hC_2, batch_count, tol);
            }
            else
            {
                unit_check_general<T>(N, N, ldc, strideC, hC_gold, hC_1, batch_count);
                unit_check_general<T>(N, N, ldc, strideC, hC_gold, hC_2, batch_count);
            }
        }

        if(arg.norm_check)
        {
            auto err1 = std::abs(
                norm_check_general<T>('F', N, N, ldc, strideC, hC_gold, hC_1, batch_count));
            auto err2 = std::abs(
                norm_check_general<T>('F', N, N, ldc, strideC, hC_gold, hC_2, batch_count));
            rocblas_error = err1 > err2 ? err1 : err2;
        }
    }

    if(arg.timing)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        for(int i = 0; i < number_cold_calls; i++)
        {
            rocblas_herk_strided_batched_fn(handle,
                                            uplo,
                                            transA,
                                            N,
                                            K,
                                            h_alpha,
                                            dA,
                                            lda,
                                            strideA,
                                            h_beta,
                                            dC,
                                            ldc,
                                            strideC,
                                            batch_count);
        }

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds
        for(int i = 0; i < number_hot_calls; i++)
        {
            rocblas_herk_strided_batched_fn(handle,
                                            uplo,
                                            transA,
                                            N,
                                            K,
                                            h_alpha,
                                            dA,
                                            lda,
                                            strideA,
                                            h_beta,
                                            dC,
                                            ldc,
                                            strideC,
                                            batch_count);
        }
        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        Arguments targ(arg);
        targ.stride_a = strideA;
        targ.stride_c = strideC;
        ArgumentModel<e_uplo,
                      e_transA,
                      e_N,
                      e_K,
                      e_alpha,
                      e_lda,
                      e_stride_a,
                      e_beta,
                      e_ldc,
                      e_stride_c,
                      e_batch_count>{}
            .log_args<T>(rocblas_cout,
                         targ,
                         gpu_time_used,
                         herk_gflop_count<T>(N, K),
                         ArgumentLogging::NA_value,
                         cpu_time_used,
                         rocblas_error);
    }
}
