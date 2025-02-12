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
#include "rocblas_solve.hpp"
#include "rocblas_test.hpp"
#include "rocblas_vector.hpp"
#include "unit.hpp"
#include "utility.hpp"

template <typename T>
void testing_tbsv_bad_arg(const Arguments& arg)
{
    auto rocblas_tbsv_fn = arg.fortran ? rocblas_tbsv<T, true> : rocblas_tbsv<T, false>;

    const rocblas_int       N      = 100;
    const rocblas_int       K      = 5;
    const rocblas_int       lda    = 100;
    const rocblas_int       incx   = 1;
    const rocblas_operation transA = rocblas_operation_none;
    const rocblas_fill      uplo   = rocblas_fill_lower;
    const rocblas_diagonal  diag   = rocblas_diagonal_non_unit;

    rocblas_local_handle handle{arg};

    size_t size_A = lda * size_t(N);
    size_t size_x = N * size_t(incx);

    device_vector<T> dA(size_A);
    device_vector<T> dx(size_x);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dx.memcheck());

    //
    // Checks.
    //
    EXPECT_ROCBLAS_STATUS(
        rocblas_tbsv_fn(handle, rocblas_fill_full, transA, diag, N, K, dA, lda, dx, incx),
        rocblas_status_invalid_value);
    EXPECT_ROCBLAS_STATUS(rocblas_tbsv_fn(handle, uplo, transA, diag, N, K, nullptr, lda, dx, incx),
                          rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS(rocblas_tbsv_fn(handle, uplo, transA, diag, N, K, dA, lda, nullptr, incx),
                          rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS(rocblas_tbsv_fn(nullptr, uplo, transA, diag, N, K, dA, lda, dx, incx),
                          rocblas_status_invalid_handle);
}

template <typename T>
void testing_tbsv(const Arguments& arg)
{
    auto rocblas_tbsv_fn = arg.fortran ? rocblas_tbsv<T, true> : rocblas_tbsv<T, false>;

    rocblas_int N           = arg.N;
    rocblas_int K           = arg.K;
    rocblas_int lda         = arg.lda;
    rocblas_int incx        = arg.incx;
    char        char_uplo   = arg.uplo;
    char        char_transA = arg.transA;
    char        char_diag   = arg.diag;

    rocblas_fill      uplo   = char2rocblas_fill(char_uplo);
    rocblas_operation transA = char2rocblas_operation(char_transA);
    rocblas_diagonal  diag   = char2rocblas_diagonal(char_diag);

    rocblas_status       status;
    rocblas_local_handle handle{arg};

    // check here to prevent undefined memory allocation error
    bool invalid_size = N < 0 || K < 0 || lda < K + 1 || !incx;
    if(invalid_size || !N)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        EXPECT_ROCBLAS_STATUS(
            rocblas_tbsv_fn(handle, uplo, transA, diag, N, K, nullptr, lda, nullptr, incx),
            invalid_size ? rocblas_status_invalid_size : rocblas_status_success);
        return;
    }

    // size_A is N*N since lda might be < N.
    size_t size_A   = size_t(N) * N;
    size_t size_AB  = size_t(lda) * N;
    size_t abs_incx = size_t(incx >= 0 ? incx : -incx);
    size_t size_x   = N * abs_incx;

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<T> hA(size_A);
    host_vector<T> hAB(size_AB);
    host_vector<T> AAT(size_A);
    host_vector<T> hb(size_x);
    host_vector<T> hx(size_x);
    host_vector<T> hx_or_b_1(size_x);
    host_vector<T> hx_or_b_2(size_x);
    host_vector<T> cpu_x_or_b(size_x);

    double gpu_time_used, cpu_time_used;
    double error_eps_multiplier    = 40.0;
    double residual_eps_multiplier = 40.0;
    double eps                     = std::numeric_limits<real_t<T>>::epsilon();

    // allocate memory on device
    device_vector<T> dAB(size_AB);
    device_vector<T> dx_or_b(size_x);
    CHECK_DEVICE_ALLOCATION(dAB.memcheck());
    CHECK_DEVICE_ALLOCATION(dx_or_b.memcheck());

    // Initialize data on host memory
    //Matrix `hA` is initialized as a general matrix because the general matrix is converted into Banded matrix by the function regular_to_banded below
    rocblas_init_matrix(hA,
                        arg,
                        size_A,
                        1,
                        1,
                        0,
                        1,
                        rocblas_client_never_set_nan,
                        rocblas_client_general_matrix,
                        true);
    rocblas_init_vector(hx, arg, N, abs_incx, 0, 1, rocblas_client_never_set_nan, false, true);

    // Make hA a banded matrix with k sub/super-diagonals
    banded_matrix_setup(uplo == rocblas_fill_upper, (T*)hA, N, N, K);

    prepare_triangular_solve((T*)hA, N, (T*)AAT, N, char_uplo);
    if(diag == rocblas_diagonal_unit)
    {
        make_unit_diagonal(uplo, (T*)hA, N, N);
    }

    // Convert regular-storage hA to banded-storage hAB
    regular_to_banded(uplo == rocblas_fill_upper, (T*)hA, N, (T*)hAB, lda, N, K);
    CHECK_HIP_ERROR(dAB.transfer_from(hAB));

    // initialize "exact" answer hx

    hb = hx;

    cblas_tbmv<T>(uplo, transA, diag, N, K, hAB, lda, hb, incx);
    cpu_x_or_b = hb;
    hx_or_b_1  = hb;
    hx_or_b_2  = hb;

    double max_err_1 = 0.0;
    double max_err_2 = 0.0;

    if(arg.unit_check || arg.norm_check)
    {
        // calculate dxorb <- A^(-1) b   rocblas_device_pointer_host
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        CHECK_HIP_ERROR(dx_or_b.transfer_from(hx_or_b_1));

        CHECK_ROCBLAS_ERROR(
            rocblas_tbsv_fn(handle, uplo, transA, diag, N, K, dAB, lda, dx_or_b, incx));

        CHECK_HIP_ERROR(hx_or_b_1.transfer_from(dx_or_b));

        // calculate dxorb <- A^(-1) b   rocblas_device_pointer_device
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_HIP_ERROR(dx_or_b.transfer_from(hx_or_b_2));

        CHECK_ROCBLAS_ERROR(
            rocblas_tbsv_fn(handle, uplo, transA, diag, N, K, dAB, lda, dx_or_b, incx));
        CHECK_HIP_ERROR(hx_or_b_2.transfer_from(dx_or_b));

        //computed result is in hx_or_b, so forward error is E = hx - hx_or_b
        // calculate norm 1 of vector E
        max_err_1 = rocblas_abs(vector_norm_1<T>(N, abs_incx, hx, hx_or_b_1));
        max_err_2 = rocblas_abs(vector_norm_1<T>(N, abs_incx, hx, hx_or_b_2));

        //unit test
        trsm_err_res_check<T>(max_err_1, N, error_eps_multiplier, eps);
        trsm_err_res_check<T>(max_err_2, N, error_eps_multiplier, eps);

        // hx_or_b contains A * (calculated X), so res = A * (calculated x) - b = hx_or_b - hb
        cblas_tbmv<T>(uplo, transA, diag, N, K, hAB, lda, hx_or_b_1, incx);
        cblas_tbmv<T>(uplo, transA, diag, N, K, hAB, lda, hx_or_b_2, incx);

        // Calculate norm 1 of vector res
        max_err_1 = rocblas_abs(vector_norm_1<T>(N, abs_incx, hx_or_b_1, hb));
        max_err_2 = rocblas_abs(vector_norm_1<T>(N, abs_incx, hx_or_b_1, hb));

        //unit test
        trsm_err_res_check<T>(max_err_1, N, residual_eps_multiplier, eps);
        trsm_err_res_check<T>(max_err_2, N, residual_eps_multiplier, eps);
    }

    if(arg.timing)
    {
        // GPU rocBLAS
        CHECK_HIP_ERROR(dx_or_b.transfer_from(hx_or_b_1));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;

        for(int i = 0; i < number_cold_calls; i++)
            rocblas_tbsv_fn(handle, uplo, transA, diag, N, K, dAB, lda, dx_or_b, incx);

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int i = 0; i < number_hot_calls; i++)
            rocblas_tbsv_fn(handle, uplo, transA, diag, N, K, dAB, lda, dx_or_b, incx);

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        // CPU cblas
        cpu_time_used = get_time_us_no_sync();

        if(arg.norm_check)
            cblas_tbsv<T>(uplo, transA, diag, N, K, hAB, lda, cpu_x_or_b, incx);

        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        ArgumentModel<e_uplo, e_transA, e_diag, e_N, e_K, e_lda, e_incx>{}.log_args<T>(
            rocblas_cout,
            arg,
            gpu_time_used,
            tbsv_gflop_count<T>(N, K),
            ArgumentLogging::NA_value,
            cpu_time_used,
            max_err_1,
            max_err_2);
    }
}
