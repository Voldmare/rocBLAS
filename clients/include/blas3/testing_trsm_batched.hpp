/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
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
void testing_trsm_batched_bad_arg(const Arguments& arg)
{
    auto rocblas_trsm_batched_fn
        = arg.fortran ? rocblas_trsm_batched<T, true> : rocblas_trsm_batched<T, false>;

    rocblas_local_handle handle{arg};
    const rocblas_int    M           = 100;
    const rocblas_int    N           = 100;
    const rocblas_int    lda         = 100;
    const rocblas_int    ldb         = 100;
    const rocblas_int    batch_count = 2;
    const T              alpha       = 1.0;
    const T              zero        = 0.0;

    const rocblas_side      side   = rocblas_side_left;
    const rocblas_fill      uplo   = rocblas_fill_upper;
    const rocblas_operation transA = rocblas_operation_none;
    const rocblas_diagonal  diag   = rocblas_diagonal_non_unit;

    // allocate memory on device
    const size_t           safe_size = 100;
    device_batch_vector<T> dA(safe_size, 1, batch_count);
    device_batch_vector<T> dB(safe_size * safe_size, 1, batch_count);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dB.memcheck());

    CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

    EXPECT_ROCBLAS_STATUS(
        rocblas_trsm_batched_fn(
            handle, side, uplo, transA, diag, M, N, &alpha, nullptr, lda, dB, ldb, batch_count),
        rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(
        rocblas_trsm_batched_fn(
            handle, side, uplo, transA, diag, M, N, &alpha, dA, lda, nullptr, ldb, batch_count),
        rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(
        rocblas_trsm_batched_fn(
            handle, side, uplo, transA, diag, M, N, nullptr, dA, lda, dB, ldb, batch_count),
        rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(
        rocblas_trsm_batched_fn(
            nullptr, side, uplo, transA, diag, M, N, &alpha, dA, lda, dB, ldb, batch_count),
        rocblas_status_invalid_handle);

    // When batch_count==0, all pointers may be nullptr without error
    EXPECT_ROCBLAS_STATUS(
        rocblas_trsm_batched_fn(
            handle, side, uplo, transA, diag, M, N, nullptr, nullptr, lda, nullptr, ldb, 0),
        rocblas_status_success);

    // When M==0, all pointers may be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                  side,
                                                  uplo,
                                                  transA,
                                                  diag,
                                                  0,
                                                  N,
                                                  nullptr,
                                                  nullptr,
                                                  lda,
                                                  nullptr,
                                                  ldb,
                                                  batch_count),
                          rocblas_status_success);

    // When N==0, all pointers may be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                  side,
                                                  uplo,
                                                  transA,
                                                  diag,
                                                  M,
                                                  0,
                                                  nullptr,
                                                  nullptr,
                                                  lda,
                                                  nullptr,
                                                  ldb,
                                                  batch_count),
                          rocblas_status_success);

    // If alpha==0, then A can be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                  side,
                                                  uplo,
                                                  transA,
                                                  diag,
                                                  M,
                                                  N,
                                                  &zero,
                                                  nullptr,
                                                  lda,
                                                  dB.ptr_on_device(),
                                                  ldb,
                                                  batch_count),
                          rocblas_status_success);
}

template <typename T>
void testing_trsm_batched(const Arguments& arg)
{
    auto rocblas_trsm_batched_fn
        = arg.fortran ? rocblas_trsm_batched<T, true> : rocblas_trsm_batched<T, false>;

    rocblas_int M           = arg.M;
    rocblas_int N           = arg.N;
    rocblas_int lda         = arg.lda;
    rocblas_int ldb         = arg.ldb;
    rocblas_int batch_count = arg.batch_count;

    char char_side   = arg.side;
    char char_uplo   = arg.uplo;
    char char_transA = arg.transA;
    char char_diag   = arg.diag;
    T    alpha_h     = arg.alpha;

    rocblas_side      side   = char2rocblas_side(char_side);
    rocblas_fill      uplo   = char2rocblas_fill(char_uplo);
    rocblas_operation transA = char2rocblas_operation(char_transA);
    rocblas_diagonal  diag   = char2rocblas_diagonal(char_diag);

    rocblas_int K      = side == rocblas_side_left ? M : N;
    size_t      size_A = lda * size_t(K);
    size_t      size_B = ldb * size_t(N);

    rocblas_local_handle handle{arg};
    // check here to prevent undefined memory allocation error
    bool invalid_size = M < 0 || N < 0 || lda < K || ldb < M || batch_count < 0;
    if(invalid_size || batch_count == 0)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        EXPECT_ROCBLAS_STATUS(rocblas_trsm_batched_fn(handle,
                                                      side,
                                                      uplo,
                                                      transA,
                                                      diag,
                                                      M,
                                                      N,
                                                      nullptr,
                                                      nullptr,
                                                      lda,
                                                      nullptr,
                                                      ldb,
                                                      batch_count),
                              invalid_size ? rocblas_status_invalid_size : rocblas_status_success);
        return;
    }

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_batch_vector<T> hA(size_A, 1, batch_count);
    host_batch_vector<T> AAT(size_A, 1, batch_count);
    host_batch_vector<T> hB(size_B, 1, batch_count);
    host_batch_vector<T> hX(size_B, 1, batch_count);
    host_batch_vector<T> hXorB_1(size_B, 1, batch_count);
    host_batch_vector<T> hXorB_2(size_B, 1, batch_count);
    host_batch_vector<T> cpuXorB(size_B, 1, batch_count);
    host_vector<T>       halpha(1);
    halpha[0] = alpha_h;

    double gpu_time_used, cpu_time_used;
    gpu_time_used = cpu_time_used  = 0.0;
    double error_eps_multiplier    = ERROR_EPS_MULTIPLIER;
    double residual_eps_multiplier = RESIDUAL_EPS_MULTIPLIER;
    double eps                     = std::numeric_limits<real_t<T>>::epsilon();

    // allocate memory on device
    device_batch_vector<T> dA(size_A, 1, batch_count);
    device_batch_vector<T> dXorB(size_B, 1, batch_count);
    device_vector<T>       alpha_d(1);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dXorB.memcheck());
    CHECK_DEVICE_ALLOCATION(alpha_d.memcheck());

    //  Random lower triangular matrices have condition number
    //  that grows exponentially with matrix size. Random full
    //  matrices have condition that grows linearly with
    //  matrix size.
    //
    //  We want a triangular matrix with condition number that grows
    //  lineary with matrix size. We start with full random matrix A.
    //  Calculate symmetric AAT <- A A^T. Make AAT strictly diagonal
    //  dominant. A strictly diagonal dominant matrix is SPD so we
    //  can use Cholesky to calculate L L^T = AAT. These L factors
    //  should have condition number approximately equal to
    //  the condition number of the original matrix A.

    // Initialize data on host memory
    rocblas_init_vector(hA, arg, rocblas_client_never_set_nan, true);
    rocblas_init_vector(hX, arg, rocblas_client_never_set_nan, false, true);

    for(int b = 0; b < batch_count; b++)
    {
        for(int i = K; i < lda; i++)
            for(int j = 0; j < K; j++)
                hA[b][i + j * lda] = 0.0;

        //  calculate AAT = hA * hA ^ T or AAT = hA * hA ^ H if complex
        cblas_gemm<T>(rocblas_operation_none,
                      rocblas_operation_conjugate_transpose,
                      K,
                      K,
                      K,
                      T(1.0),
                      hA[b],
                      lda,
                      hA[b],
                      lda,
                      T(0.0),
                      AAT[b],
                      lda);

        //  copy AAT into hA, make hA strictly diagonal dominant, and therefore SPD
        for(int i = 0; i < K; i++)
        {
            T t = 0.0;
            for(int j = 0; j < K; j++)
            {
                int idx    = i + j * lda;
                hA[b][idx] = AAT[b][idx];
                t += rocblas_abs(AAT[b][idx]);
            }
            hA[b][i + i * lda] = t;
        }

        //  calculate Cholesky factorization of SPD (or Hermitian if complex) matrix hA
        cblas_potrf<T>(char_uplo, K, hA[b], lda);
    }

    //  make hA unit diagonal if diag == rocblas_diagonal_unit
    if(char_diag == 'U' || char_diag == 'u')
    {
        if('L' == char_uplo || 'l' == char_uplo)
        {
            for(int b = 0; b < batch_count; b++)
            {
                for(int i = 0; i < K; i++)
                {
                    T diag = hA[b][i + i * lda];
                    for(int j = 0; j <= i; j++)
                        hA[b][i + j * lda] = hA[b][i + j * lda] / diag;
                }
            }
        }
        else
        {
            for(int b = 0; b < batch_count; b++)
            {
                for(int j = 0; j < K; j++)
                {
                    T diag = hA[b][j + j * lda];
                    for(int i = 0; i <= j; i++)
                        hA[b][i + j * lda] = hA[b][i + j * lda] / diag;
                }
            }
        }
    }

    // Initialize "exact" answer hx
    for(int b = 0; b < batch_count; b++)
    {
        for(int i = M; i < ldb; i++)
            for(int j = 0; j < N; j++)
                hX[b][i + j * ldb] = 0.0;
    }

    hB.copy_from(hX);

    for(int b = 0; b < batch_count; b++)
    {
        // Calculate hB = hA*hX
        cblas_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA[b], lda, hB[b], ldb);
    }

    hXorB_1.copy_from(hB);
    hXorB_2.copy_from(hB);
    cpuXorB.copy_from(hB);

    CHECK_HIP_ERROR(dA.transfer_from(hA));
    CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

    double max_err_1 = 0.0;
    double max_err_2 = 0.0;

    if(!ROCBLAS_REALLOC_ON_DEMAND)
    {
        // Compute size
        CHECK_ROCBLAS_ERROR(rocblas_start_device_memory_size_query(handle));
        CHECK_ALLOC_QUERY(rocblas_trsm_batched_fn(handle,
                                                  side,
                                                  uplo,
                                                  transA,
                                                  diag,
                                                  M,
                                                  N,
                                                  &alpha_h,
                                                  dA.ptr_on_device(),
                                                  lda,
                                                  dXorB.ptr_on_device(),
                                                  ldb,
                                                  batch_count));
        size_t size;
        CHECK_ROCBLAS_ERROR(rocblas_stop_device_memory_size_query(handle, &size));

        // Allocate memory
        CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, size));
    }

    if(arg.unit_check || arg.norm_check)
    {
        // calculate dXorB <- A^(-1) B   rocblas_device_pointer_host
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

        CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                    side,
                                                    uplo,
                                                    transA,
                                                    diag,
                                                    M,
                                                    N,
                                                    &alpha_h,
                                                    dA.ptr_on_device(),
                                                    lda,
                                                    dXorB.ptr_on_device(),
                                                    ldb,
                                                    batch_count));

        CHECK_HIP_ERROR(hXorB_1.transfer_from(dXorB));

        // calculate dXorB <- A^(-1) B   rocblas_device_pointer_device
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_HIP_ERROR(alpha_d.transfer_from(halpha));
        CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_2));

        CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                    side,
                                                    uplo,
                                                    transA,
                                                    diag,
                                                    M,
                                                    N,
                                                    alpha_d,
                                                    dA.ptr_on_device(),
                                                    lda,
                                                    dXorB.ptr_on_device(),
                                                    ldb,
                                                    batch_count));

        CHECK_HIP_ERROR(hXorB_2.transfer_from(dXorB));

        if(alpha_h == 0)
        {
            // expecting 0 output, set hX == 0
            for(rocblas_int b = 0; b < batch_count; b++)
                for(rocblas_int i = 0; i < M; i++)
                    for(rocblas_int j = 0; j < N; j++)
                        hX[b][i + j * ldb] = 0.0;

            if(arg.unit_check)
            {
                unit_check_general<T>(M, N, ldb, hX, hXorB_1, batch_count);
                unit_check_general<T>(M, N, ldb, hX, hXorB_2, batch_count);
            }

            if(arg.norm_check)
            {
                max_err_1
                    = std::abs(norm_check_general<T>('F', M, N, ldb, hX, hXorB_1, batch_count));
                max_err_2
                    = std::abs(norm_check_general<T>('F', M, N, ldb, hX, hXorB_2, batch_count));
            }
        }
        else
        {
            //computed result is in hx_or_b, so forward error is E = hx - hx_or_b
            // calculate vector-induced-norm 1 of matrix E
            for(int b = 0; b < batch_count; b++)
            {
                max_err_1 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hX[b], hXorB_1[b]));
                max_err_2 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hX[b], hXorB_2[b]));

                //unit test
                trsm_err_res_check<T>(max_err_1, M, error_eps_multiplier, eps);
                trsm_err_res_check<T>(max_err_2, M, error_eps_multiplier, eps);

                // hx_or_b contains A * (calculated X), so res = A * (calculated x) - b = hx_or_b - hb
                cblas_trmm<T>(
                    side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA[b], lda, hXorB_1[b], ldb);
                cblas_trmm<T>(
                    side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA[b], lda, hXorB_2[b], ldb);

                // calculate vector-induced-norm 1 of matrix res
                max_err_1 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hXorB_1[b], hB[b]));
                max_err_2 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hXorB_2[b], hB[b]));

                //unit test
                trsm_err_res_check<T>(max_err_1, M, residual_eps_multiplier, eps);
                trsm_err_res_check<T>(max_err_2, M, residual_eps_multiplier, eps);
            }
        }
    }

    if(arg.timing)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;

        // GPU rocBLAS
        CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int i = 0; i < number_cold_calls; i++)
        {
            CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                        side,
                                                        uplo,
                                                        transA,
                                                        diag,
                                                        M,
                                                        N,
                                                        &alpha_h,
                                                        dA.ptr_on_device(),
                                                        lda,
                                                        dXorB.ptr_on_device(),
                                                        ldb,
                                                        batch_count));
        }

        gpu_time_used = get_time_us_sync(stream);

        for(int i = 0; i < number_hot_calls; i++)
        {
            CHECK_ROCBLAS_ERROR(rocblas_trsm_batched_fn(handle,
                                                        side,
                                                        uplo,
                                                        transA,
                                                        diag,
                                                        M,
                                                        N,
                                                        &alpha_h,
                                                        dA.ptr_on_device(),
                                                        lda,
                                                        dXorB.ptr_on_device(),
                                                        ldb,
                                                        batch_count));
        }

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        // CPU cblas
        cpu_time_used = get_time_us_no_sync();

        for(int b = 0; b < batch_count; b++)
            cblas_trsm<T>(side, uplo, transA, diag, M, N, alpha_h, hA[b], lda, cpuXorB[b], ldb);

        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        ArgumentModel<e_side,
                      e_uplo,
                      e_transA,
                      e_diag,
                      e_M,
                      e_N,
                      e_alpha,
                      e_lda,
                      e_ldb,
                      e_batch_count>{}
            .log_args<T>(rocblas_cout,
                         arg,
                         gpu_time_used,
                         trsm_gflop_count<T>(M, N, K),
                         ArgumentLogging::NA_value,
                         cpu_time_used,
                         max_err_1,
                         max_err_2);
    }
}
