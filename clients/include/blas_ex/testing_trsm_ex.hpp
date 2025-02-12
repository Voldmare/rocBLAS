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
#define TRSM_BLOCK 128

template <typename T>
void printMatrix(const char* name, T* A, rocblas_int m, rocblas_int n, rocblas_int lda)
{
    rocblas_cout << "---------- " << name << " ----------\n";
    int max_size = 3;
    for(int i = 0; i < m; i++)
    {
        for(int j = 0; j < n; j++)
            rocblas_cout << A[i + j * lda] << " ";
        rocblas_cout << "\n";
    }
}

template <typename T>
void testing_trsm_ex_bad_arg(const Arguments& arg)
{
    auto rocblas_trsm_ex_fn = arg.fortran ? rocblas_trsm_ex_fortran : rocblas_trsm_ex;

    const rocblas_int M   = 100;
    const rocblas_int N   = 100;
    const rocblas_int lda = 100;
    const rocblas_int ldb = 100;

    const T alpha = 1.0;
    const T zero  = 0.0;

    const rocblas_side      side   = rocblas_side_left;
    const rocblas_fill      uplo   = rocblas_fill_upper;
    const rocblas_operation transA = rocblas_operation_none;
    const rocblas_diagonal  diag   = rocblas_diagonal_non_unit;

    rocblas_local_handle handle{arg};

    rocblas_int K        = side == rocblas_side_left ? M : N;
    size_t      size_A   = lda * size_t(K);
    size_t      size_B   = ldb * size_t(N);
    size_t      sizeInvA = TRSM_BLOCK * K;

    // allocate memory on device
    device_vector<T> dA(size_A);
    device_vector<T> dB(size_B);
    device_vector<T> dinvA(sizeInvA);

    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dB.memcheck());

    CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
                                             side,
                                             uplo,
                                             transA,
                                             diag,
                                             M,
                                             N,
                                             &alpha,
                                             nullptr,
                                             lda,
                                             dB,
                                             ldb,
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_f32_r),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
                                             side,
                                             uplo,
                                             transA,
                                             diag,
                                             M,
                                             N,
                                             &alpha,
                                             dA,
                                             lda,
                                             nullptr,
                                             ldb,
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_f32_r),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
                                             side,
                                             uplo,
                                             transA,
                                             diag,
                                             M,
                                             N,
                                             nullptr,
                                             dA,
                                             lda,
                                             dB,
                                             ldb,
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_f32_r),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(nullptr,
                                             side,
                                             uplo,
                                             transA,
                                             diag,
                                             M,
                                             N,
                                             &alpha,
                                             dA,
                                             lda,
                                             dB,
                                             ldb,
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_f32_r),
                          rocblas_status_invalid_handle);

    // If M==0, then all pointers can be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
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
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_f32_r),
                          rocblas_status_success);

    // If N==0, then all pointers can be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
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
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_f32_r),
                          rocblas_status_success);

    // If alpha==0, then A can be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
                                             side,
                                             uplo,
                                             transA,
                                             diag,
                                             M,
                                             N,
                                             &zero,
                                             nullptr,
                                             lda,
                                             dB,
                                             ldb,
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_f32_r),
                          rocblas_status_success);

    // Unsupported datatype
    EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
                                             side,
                                             uplo,
                                             transA,
                                             diag,
                                             M,
                                             N,
                                             &alpha,
                                             dA,
                                             lda,
                                             dB,
                                             ldb,
                                             dinvA,
                                             sizeInvA,
                                             rocblas_datatype_bf16_r),
                          rocblas_status_not_implemented);
}

template <typename T>
void testing_trsm_ex(const Arguments& arg)
{
    auto rocblas_trsm_ex_fn = arg.fortran ? rocblas_trsm_ex_fortran : rocblas_trsm_ex;

    rocblas_int M   = arg.M;
    rocblas_int N   = arg.N;
    rocblas_int lda = arg.lda;
    rocblas_int ldb = arg.ldb;

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
    bool invalid_size = M < 0 || N < 0 || lda < K || ldb < M;
    if(invalid_size)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        EXPECT_ROCBLAS_STATUS(rocblas_trsm_ex_fn(handle,
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
                                                 nullptr,
                                                 TRSM_BLOCK * K,
                                                 arg.compute_type),
                              rocblas_status_invalid_size);
        return;
    }

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<T> hA(size_A);
    host_vector<T> AAT(size_A);
    host_vector<T> hB(size_B);
    host_vector<T> hX(size_B);
    host_vector<T> hXorB_1(size_B);
    host_vector<T> hXorB_2(size_B);
    host_vector<T> cpuXorB(size_B);
    host_vector<T> invATemp1(TRSM_BLOCK * K);
    host_vector<T> invATemp2(TRSM_BLOCK * K);
    host_vector<T> hinvAI(TRSM_BLOCK * K);

    double gpu_time_used, cpu_time_used;
    gpu_time_used = cpu_time_used  = 0.0;
    double error_eps_multiplier    = ERROR_EPS_MULTIPLIER;
    double residual_eps_multiplier = RESIDUAL_EPS_MULTIPLIER;
    double eps                     = std::numeric_limits<real_t<T>>::epsilon();

    // allocate memory on device
    device_vector<T> dA(size_A);
    device_vector<T> dXorB(size_B);
    device_vector<T> alpha_d(1);
    device_vector<T> dinvA(TRSM_BLOCK * K);
    device_vector<T> dX_tmp(M * N);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dXorB.memcheck());
    CHECK_DEVICE_ALLOCATION(alpha_d.memcheck());
    CHECK_DEVICE_ALLOCATION(dinvA.memcheck());
    CHECK_DEVICE_ALLOCATION(dX_tmp.memcheck());

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
    rocblas_init_matrix(hA,
                        arg,
                        K,
                        K,
                        lda,
                        0,
                        1,
                        rocblas_client_never_set_nan,
                        rocblas_client_triangular_matrix,
                        true);

    rocblas_init_matrix(hX,
                        arg,
                        M,
                        N,
                        ldb,
                        0,
                        1,
                        rocblas_client_never_set_nan,
                        rocblas_client_general_matrix,
                        false,
                        true);
    hB = hX;

    //  calculate AAT = hA * hA ^ T or AAT = hA * hA ^ H if complex
    cblas_gemm<T>(rocblas_operation_none,
                  rocblas_operation_conjugate_transpose,
                  K,
                  K,
                  K,
                  T(1.0),
                  hA,
                  lda,
                  hA,
                  lda,
                  T(0.0),
                  AAT,
                  lda);

    //  copy AAT into hA, make hA strictly diagonal dominant, and therefore SPD
    for(int i = 0; i < K; i++)
    {
        T t = 0.0;
        for(int j = 0; j < K; j++)
        {
            hA[i + j * lda] = AAT[i + j * lda];
            t += rocblas_abs(AAT[i + j * lda]);
        }
        hA[i + i * lda] = t;
    }

    //  calculate Cholesky factorization of SPD (or Hermitian if complex) matrix hA
    cblas_potrf<T>(char_uplo, K, hA, lda);

    //  make hA unit diagonal if diag == rocblas_diagonal_unit
    if(char_diag == 'U' || char_diag == 'u')
    {
        if('L' == char_uplo || 'l' == char_uplo)
            for(int i = 0; i < K; i++)
            {
                T diag = hA[i + i * lda];
                for(int j = 0; j <= i; j++)
                    hA[i + j * lda] = hA[i + j * lda] / diag;
            }
        else
            for(int j = 0; j < K; j++)
            {
                T diag = hA[j + j * lda];
                for(int i = 0; i <= j; i++)
                    hA[i + j * lda] = hA[i + j * lda] / diag;
            }
    }

    // Calculate hB = hA*hX;
    cblas_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA, lda, hB, ldb);

    hXorB_1 = hB; // hXorB <- B
    hXorB_2 = hB; // hXorB <- B
    cpuXorB = hB; // cpuXorB <- B

    // copy data from CPU to device
    CHECK_HIP_ERROR(hipMemcpy(dA, hA, sizeof(T) * size_A, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dXorB, hXorB_1, sizeof(T) * size_B, hipMemcpyHostToDevice));

    rocblas_int stride_A    = TRSM_BLOCK * lda + TRSM_BLOCK;
    rocblas_int stride_invA = TRSM_BLOCK * TRSM_BLOCK;

    int blocks = K / TRSM_BLOCK;

    double max_err_1 = 0.0;
    double max_err_2 = 0.0;

    if(!ROCBLAS_REALLOC_ON_DEMAND)
    {
        // Compute size
        CHECK_ROCBLAS_ERROR(rocblas_start_device_memory_size_query(handle));

        if(blocks > 0)
        {
            CHECK_ALLOC_QUERY(rocblas_trtri_strided_batched<T>(handle,
                                                               uplo,
                                                               diag,
                                                               TRSM_BLOCK,
                                                               dA,
                                                               lda,
                                                               stride_A,
                                                               dinvA,
                                                               TRSM_BLOCK,
                                                               stride_invA,
                                                               blocks));
        }

        if(K % TRSM_BLOCK != 0 || blocks == 0)
        {
            CHECK_ALLOC_QUERY(rocblas_trtri_strided_batched<T>(handle,
                                                               uplo,
                                                               diag,
                                                               K - TRSM_BLOCK * blocks,
                                                               dA + stride_A * blocks,
                                                               lda,
                                                               stride_A,
                                                               dinvA + stride_invA * blocks,
                                                               TRSM_BLOCK,
                                                               stride_invA,
                                                               1));
        }

        CHECK_ALLOC_QUERY(rocblas_trsm_ex_fn(handle,
                                             side,
                                             uplo,
                                             transA,
                                             diag,
                                             M,
                                             N,
                                             &alpha_h,
                                             dA,
                                             lda,
                                             dXorB,
                                             ldb,
                                             dinvA,
                                             TRSM_BLOCK * K,
                                             arg.compute_type));

        size_t size;
        CHECK_ROCBLAS_ERROR(rocblas_stop_device_memory_size_query(handle, &size));

        // Allocate memory
        CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, size));
    }

    if(arg.unit_check || arg.norm_check)
    {
        // calculate dXorB <- A^(-1) B   rocblas_device_pointer_host
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        CHECK_HIP_ERROR(hipMemcpy(dXorB, hXorB_1, sizeof(T) * size_B, hipMemcpyHostToDevice));

        hipStream_t rocblas_stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &rocblas_stream));

        if(blocks > 0)
            CHECK_ROCBLAS_ERROR(rocblas_trtri_strided_batched<T>(handle,
                                                                 uplo,
                                                                 diag,
                                                                 TRSM_BLOCK,
                                                                 dA,
                                                                 lda,
                                                                 stride_A,
                                                                 dinvA,
                                                                 TRSM_BLOCK,
                                                                 stride_invA,
                                                                 blocks));

        if(K % TRSM_BLOCK != 0 || blocks == 0)
            CHECK_ROCBLAS_ERROR(rocblas_trtri_strided_batched<T>(handle,
                                                                 uplo,
                                                                 diag,
                                                                 K - TRSM_BLOCK * blocks,
                                                                 dA + stride_A * blocks,
                                                                 lda,
                                                                 stride_A,
                                                                 dinvA + stride_invA * blocks,
                                                                 TRSM_BLOCK,
                                                                 stride_invA,
                                                                 1));

        CHECK_ROCBLAS_ERROR(rocblas_trsm_ex_fn(handle,
                                               side,
                                               uplo,
                                               transA,
                                               diag,
                                               M,
                                               N,
                                               &alpha_h,
                                               dA,
                                               lda,
                                               dXorB,
                                               ldb,
                                               dinvA,
                                               TRSM_BLOCK * K,
                                               arg.compute_type));

        CHECK_HIP_ERROR(hipMemcpy(hXorB_1, dXorB, sizeof(T) * size_B, hipMemcpyDeviceToHost));

        // calculate dXorB <- A^(-1) B   rocblas_device_pointer_device
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_HIP_ERROR(hipMemcpy(dXorB, hXorB_2, sizeof(T) * size_B, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(alpha_d, &alpha_h, sizeof(T), hipMemcpyHostToDevice));

        CHECK_ROCBLAS_ERROR(rocblas_trsm_ex_fn(handle,
                                               side,
                                               uplo,
                                               transA,
                                               diag,
                                               M,
                                               N,
                                               alpha_d,
                                               dA,
                                               lda,
                                               dXorB,
                                               ldb,
                                               dinvA,
                                               TRSM_BLOCK * K,
                                               arg.compute_type));

        CHECK_HIP_ERROR(hipMemcpy(hXorB_2, dXorB, sizeof(T) * size_B, hipMemcpyDeviceToHost));

        //computed result is in hx_or_b, so forward error is E = hx - hx_or_b
        // calculate vector-induced-norm 1 of matrix E
        max_err_1 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hX, hXorB_1));
        max_err_2 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hX, hXorB_2));

        //unit test
        trsm_err_res_check<T>(max_err_1, M, error_eps_multiplier, eps);
        trsm_err_res_check<T>(max_err_2, M, error_eps_multiplier, eps);

        // hx_or_b contains A * (calculated X), so res = A * (calculated x) - b = hx_or_b - hb
        cblas_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA, lda, hXorB_1, ldb);
        cblas_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA, lda, hXorB_2, ldb);

        max_err_1 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hXorB_1, hB));
        max_err_2 = rocblas_abs(matrix_norm_1<T>(M, N, ldb, hXorB_2, hB));

        //unit test
        trsm_err_res_check<T>(max_err_1, M, residual_eps_multiplier, eps);
        trsm_err_res_check<T>(max_err_2, M, residual_eps_multiplier, eps);
    }

    if(arg.timing)
    {
        // GPU rocBLAS
        CHECK_HIP_ERROR(hipMemcpy(dXorB, hXorB_1, sizeof(T) * size_B, hipMemcpyHostToDevice));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        CHECK_ROCBLAS_ERROR(
            rocblas_trsm<T>(handle, side, uplo, transA, diag, M, N, &alpha_h, dA, lda, dXorB, ldb));

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        // CPU cblas
        cpu_time_used = get_time_us_no_sync();

        cblas_trsm<T>(side, uplo, transA, diag, M, N, alpha_h, hA, lda, cpuXorB, ldb);

        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        ArgumentModel<e_side, e_uplo, e_transA, e_diag, e_M, e_N, e_alpha, e_lda, e_ldb>{}
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
