/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "cblas_interface.hpp"
#include "flops.hpp"
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

#define ERROR_EPS_MULTIPLIER 40
#define RESIDUAL_EPS_MULTIPLIER 40

template <typename T>
void testing_trsv_strided_batched(const Arguments& arg)
{
    auto rocblas_trsv_strided_batched_fn = arg.fortran ? rocblas_trsv_strided_batched<T, true>
                                                       : rocblas_trsv_strided_batched<T, false>;

    rocblas_int M           = arg.M;
    rocblas_int lda         = arg.lda;
    rocblas_int incx        = arg.incx;
    char        char_uplo   = arg.uplo;
    char        char_transA = arg.transA;
    char        char_diag   = arg.diag;
    rocblas_int stride_a    = arg.stride_a;
    rocblas_int stride_x    = arg.stride_x;
    rocblas_int batch_count = arg.batch_count;

    rocblas_fill      uplo   = char2rocblas_fill(char_uplo);
    rocblas_operation transA = char2rocblas_operation(char_transA);
    rocblas_diagonal  diag   = char2rocblas_diagonal(char_diag);

    rocblas_status       status;
    rocblas_local_handle handle{arg};

    // check here to prevent undefined memory allocation error
    bool invalid_size = M < 0 || lda < M || lda < 1 || !incx || batch_count < 0;
    if(invalid_size || !M || !batch_count)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        EXPECT_ROCBLAS_STATUS(rocblas_trsv_strided_batched_fn(handle,
                                                              uplo,
                                                              transA,
                                                              diag,
                                                              M,
                                                              nullptr,
                                                              lda,
                                                              stride_a,
                                                              nullptr,
                                                              incx,
                                                              stride_x,
                                                              batch_count),
                              invalid_size ? rocblas_status_invalid_size : rocblas_status_success);
        return;
    }

    size_t size_A   = lda * size_t(M) + stride_a * (batch_count - 1);
    size_t abs_incx = size_t(incx >= 0 ? incx : -incx);
    size_t size_x   = M * abs_incx + stride_x * (batch_count - 1);

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<T> hA(size_A);
    host_vector<T> AAT(size_A);
    host_vector<T> hb(size_x);
    host_vector<T> hx(size_x);
    host_vector<T> hx_or_b_1(size_x);
    host_vector<T> hx_or_b_2(size_x);
    host_vector<T> cpu_x_or_b(size_x);

    double gpu_time_used, cpu_time_used;
    double error_eps_multiplier    = ERROR_EPS_MULTIPLIER;
    double residual_eps_multiplier = RESIDUAL_EPS_MULTIPLIER;
    double eps                     = std::numeric_limits<real_t<T>>::epsilon();

    // allocate memory on device
    device_vector<T> dA(size_A);
    device_vector<T> dx_or_b(size_x);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dx_or_b.memcheck());

    // Initialize data on host memory
    rocblas_init_matrix(hA,
                        arg,
                        M,
                        M,
                        lda,
                        stride_a,
                        batch_count,
                        rocblas_client_never_set_nan,
                        rocblas_client_triangular_matrix,
                        true);
    rocblas_init_vector(
        hx, arg, M, abs_incx, stride_x, batch_count, rocblas_client_never_set_nan, false, true);

    //  calculate AAT = hA * hA ^ T or AAT = hA * hA ^ H if complex
    for(int b = 0; b < batch_count; b++)
    {
        cblas_gemm<T>(rocblas_operation_none,
                      rocblas_operation_conjugate_transpose,
                      M,
                      M,
                      M,
                      T(1.0),
                      hA + stride_a * b,
                      lda,
                      hA + stride_a * b,
                      lda,
                      T(0.0),
                      AAT + stride_a * b,
                      lda);

        //  copy AAT into hA, make hA strictly diagonal dominant, and therefore SPD
        for(int i = 0; i < M; i++)
        {
            T t = 0.0;
            for(int j = 0; j < M; j++)
            {
                int idx = i + j * lda + b * stride_a;
                hA[idx] = AAT[idx];
                t += rocblas_abs(AAT[idx]);
            }
            hA[i + i * lda + b * stride_a] = t;
        }

        //  calculate Cholesky factorization of SPD (or Hermitian if complex) matrix hA
        cblas_potrf<T>(char_uplo, M, hA + stride_a * b, lda);

        //  make hA unit diagonal if diag == rocblas_diagonal_unit
        if(char_diag == 'U' || char_diag == 'u')
        {
            if('L' == char_uplo || 'l' == char_uplo)
            {
                for(int i = 0; i < M; i++)
                {
                    T diag = hA[i + i * lda + stride_a * b];
                    for(int j = 0; j <= i; j++)
                        hA[i + j * lda + stride_a * b] = hA[i + j * lda + stride_a * b] / diag;
                }
            }
            else
            {
                for(int j = 0; j < M; j++)
                {
                    T diag = hA[j + j * lda + stride_a * b];
                    for(int i = 0; i <= j; i++)
                        hA[i + j * lda + stride_a * b] = hA[i + j * lda + stride_a * b] / diag;
                }
            }
        }
    }

    hb = hx;

    // Calculate hb = hA*hx;
    for(int b = 0; b < batch_count; b++)
    {
        cblas_trmv<T>(uplo, transA, diag, M, hA + stride_a * b, lda, hb + stride_x * b, incx);
    }
    cpu_x_or_b = hb; // cpuXorB <- B
    hx_or_b_1  = hb;
    hx_or_b_2  = hb;

    // copy data from CPU to device
    CHECK_HIP_ERROR(hipMemcpy(dA, hA, sizeof(T) * size_A, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dx_or_b, hx_or_b_1, sizeof(T) * size_x, hipMemcpyHostToDevice));

    double error_host       = 0.0;
    double error_device     = 0.0;
    double max_error_host   = 0.0;
    double max_error_device = 0.0;

    if(!ROCBLAS_REALLOC_ON_DEMAND)
    {
        // Compute size
        CHECK_ROCBLAS_ERROR(rocblas_start_device_memory_size_query(handle));

        CHECK_ALLOC_QUERY(rocblas_trsv_strided_batched_fn(handle,
                                                          uplo,
                                                          transA,
                                                          diag,
                                                          M,
                                                          dA,
                                                          lda,
                                                          stride_a,
                                                          dx_or_b,
                                                          incx,
                                                          stride_x,
                                                          batch_count));
        size_t size;
        CHECK_ROCBLAS_ERROR(rocblas_stop_device_memory_size_query(handle, &size));

        // Allocate memory
        CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, size));
    }

    if(arg.unit_check || arg.norm_check)
    {
        // calculate dxorb <- A^(-1) b   rocblas_device_pointer_host
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        CHECK_ROCBLAS_ERROR(rocblas_trsv_strided_batched_fn(handle,
                                                            uplo,
                                                            transA,
                                                            diag,
                                                            M,
                                                            dA,
                                                            lda,
                                                            stride_a,
                                                            dx_or_b,
                                                            incx,
                                                            stride_x,
                                                            batch_count));
        CHECK_HIP_ERROR(hipMemcpy(hx_or_b_1, dx_or_b, sizeof(T) * size_x, hipMemcpyDeviceToHost));

        // calculate dxorb <- A^(-1) b   rocblas_device_pointer_device
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_HIP_ERROR(hipMemcpy(dx_or_b, hx_or_b_2, sizeof(T) * size_x, hipMemcpyHostToDevice));
        CHECK_ROCBLAS_ERROR(rocblas_trsv_strided_batched_fn(handle,
                                                            uplo,
                                                            transA,
                                                            diag,
                                                            M,
                                                            dA,
                                                            lda,
                                                            stride_a,
                                                            dx_or_b,
                                                            incx,
                                                            stride_x,
                                                            batch_count));
        CHECK_HIP_ERROR(hipMemcpy(hx_or_b_2, dx_or_b, sizeof(T) * size_x, hipMemcpyDeviceToHost));

        //computed result is in hx_or_b, so forward error is E = hx - hx_or_b
        // calculate norm 1 of vector E
        for(int b = 0; b < batch_count; b++)
        {
            error_host = rocblas_abs(
                vector_norm_1<T>(M, abs_incx, &hx[b * stride_x], &hx_or_b_1[b * stride_x]));
            error_device = rocblas_abs(
                vector_norm_1<T>(M, abs_incx, &hx[b * stride_x], &hx_or_b_2[b * stride_x]));
            max_error_host   = std::max(max_error_host, error_host);
            max_error_device = std::max(max_error_device, error_device);

            //unit test
            trsm_err_res_check<T>(error_host, M, error_eps_multiplier, eps);
            trsm_err_res_check<T>(error_device, M, error_eps_multiplier, eps);
        }

        // hx_or_b contains A * (calculated X), so res = A * (calculated x) - b = hx_or_b - hb
        for(int b = 0; b < batch_count; b++)
        {
            cblas_trmv<T>(
                uplo, transA, diag, M, hA + b * stride_a, lda, hx_or_b_1 + b * stride_x, incx);
            cblas_trmv<T>(
                uplo, transA, diag, M, hA + b * stride_a, lda, hx_or_b_2 + b * stride_x, incx);
        }

        //calculate norm 1 of res
        for(int b = 0; b < batch_count; b++)
        {
            error_host = rocblas_abs(
                vector_norm_1<T>(M, abs_incx, &hx_or_b_1[b * stride_x], &hb[b * stride_x]));
            error_device = rocblas_abs(
                vector_norm_1<T>(M, abs_incx, &hx_or_b_1[b * stride_x], &hb[b * stride_x]));

            //unit test
            trsm_err_res_check<T>(error_host, M, error_eps_multiplier, eps);
            trsm_err_res_check<T>(error_device, M, error_eps_multiplier, eps);
        }
    }

    if(arg.timing)
    {
        // GPU rocBLAS
        CHECK_HIP_ERROR(hipMemcpy(dx_or_b, hx_or_b_1, sizeof(T) * size_x, hipMemcpyHostToDevice));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;

        for(int i = 0; i < number_cold_calls; i++)
            rocblas_trsv_strided_batched_fn(handle,
                                            uplo,
                                            transA,
                                            diag,
                                            M,
                                            dA,
                                            lda,
                                            stride_a,
                                            dx_or_b,
                                            incx,
                                            stride_x,
                                            batch_count);

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int i = 0; i < number_hot_calls; i++)
            rocblas_trsv_strided_batched_fn(handle,
                                            uplo,
                                            transA,
                                            diag,
                                            M,
                                            dA,
                                            lda,
                                            stride_a,
                                            dx_or_b,
                                            incx,
                                            stride_x,
                                            batch_count);

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        // CPU cblas
        cpu_time_used = get_time_us_no_sync();

        if(arg.norm_check)
            for(int b = 0; b < batch_count; b++)
                cblas_trsv<T>(
                    uplo, transA, diag, M, hA + b * stride_a, lda, cpu_x_or_b + b * stride_x, incx);

        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        ArgumentModel<e_uplo,
                      e_transA,
                      e_diag,
                      e_M,
                      e_lda,
                      e_stride_a,
                      e_incx,
                      e_stride_x,
                      e_batch_count>{}
            .log_args<T>(rocblas_cout,
                         arg,
                         gpu_time_used,
                         trsv_gflop_count<T>(M),
                         ArgumentLogging::NA_value,
                         cpu_time_used,
                         max_error_host,
                         max_error_device);
    }
}
