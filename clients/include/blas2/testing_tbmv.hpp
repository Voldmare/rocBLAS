/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 *
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
void testing_tbmv_bad_arg(const Arguments& arg)
{
    auto rocblas_tbmv_fn = arg.fortran ? rocblas_tbmv<T, true> : rocblas_tbmv<T, false>;

    const rocblas_int M    = 100;
    const rocblas_int K    = 5;
    const rocblas_int lda  = 100;
    const rocblas_int incx = 1;

    const rocblas_fill      uplo   = rocblas_fill_upper;
    const rocblas_operation transA = rocblas_operation_none;
    const rocblas_diagonal  diag   = rocblas_diagonal_non_unit;

    rocblas_local_handle handle{arg};

    size_t size_A = lda * size_t(M);
    size_t size_x = M * size_t(incx);

    // allocate memory on device
    device_vector<T> dA(size_A);
    device_vector<T> dx(size_x);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dx.memcheck());

    EXPECT_ROCBLAS_STATUS(rocblas_tbmv_fn(handle, uplo, transA, diag, M, K, nullptr, lda, dx, incx),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_tbmv_fn(handle, uplo, transA, diag, M, K, dA, lda, nullptr, incx),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_tbmv_fn(nullptr, uplo, transA, diag, M, K, dA, lda, dx, incx),
                          rocblas_status_invalid_handle);
}

template <typename T>
void testing_tbmv(const Arguments& arg)
{
    auto rocblas_tbmv_fn = arg.fortran ? rocblas_tbmv<T, true> : rocblas_tbmv<T, false>;

    rocblas_int       M                 = arg.M;
    rocblas_int       K                 = arg.K;
    rocblas_int       lda               = arg.lda;
    rocblas_int       incx              = arg.incx;
    char              char_uplo         = arg.uplo;
    char              char_diag         = arg.diag;
    rocblas_fill      uplo              = char2rocblas_fill(char_uplo);
    rocblas_operation transA            = char2rocblas_operation(arg.transA);
    rocblas_diagonal  diag              = char2rocblas_diagonal(char_diag);
    rocblas_int       banded_matrix_row = K + 1;

    rocblas_local_handle handle{arg};

    // argument sanity check before allocating invalid memory
    bool invalid_size = M < 0 || K < 0 || lda < banded_matrix_row || !incx;
    if(invalid_size)
    {
        EXPECT_ROCBLAS_STATUS(
            rocblas_tbmv_fn(handle, uplo, transA, diag, M, K, nullptr, lda, nullptr, incx),
            rocblas_status_invalid_size);

        return;
    }

    size_t size_A = lda * size_t(M);
    size_t size_x, abs_incx;

    abs_incx = incx >= 0 ? incx : -incx;
    size_x   = M * abs_incx;

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<T> hA(size_A);
    host_vector<T> hx(size_x);
    host_vector<T> hx_1(size_x);
    host_vector<T> hx_gold(size_x);

    device_vector<T> dA(size_A);
    device_vector<T> dx(size_x);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dx.memcheck());

    // Initialize data on host memory
    rocblas_init_matrix(hA,
                        arg,
                        banded_matrix_row,
                        M,
                        lda,
                        0,
                        1,
                        rocblas_client_never_set_nan,
                        rocblas_client_triangular_matrix,
                        true);
    rocblas_init_vector(hx, arg, M, abs_incx, 0, 1, rocblas_client_never_set_nan, false, true);

    hx_gold = hx;

    // copy data from CPU to device
    CHECK_HIP_ERROR(hipMemcpy(dA, hA, sizeof(T) * size_A, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dx, hx, sizeof(T) * size_x, hipMemcpyHostToDevice));

    double gpu_time_used, cpu_time_used;
    double rocblas_error_1;
    double rocblas_error_2;

    /* =====================================================================
           ROCBLAS
    =================================================================== */

    if(arg.unit_check || arg.norm_check)
    {
        // pointer mode shouldn't matter here
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_ROCBLAS_ERROR(rocblas_tbmv_fn(handle, uplo, transA, diag, M, K, dA, lda, dx, incx));

        // CPU BLAS
        cpu_time_used = get_time_us_no_sync();
        cblas_tbmv<T>(uplo, transA, diag, M, K, hA, lda, hx_gold, incx);
        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        // copy output from device to CPU
        CHECK_HIP_ERROR(hipMemcpy(hx_1, dx, sizeof(T) * size_x, hipMemcpyDeviceToHost));

        if(arg.unit_check)
        {
            unit_check_general<T>(1, M, abs_incx, hx_gold, hx_1);
        }

        if(arg.norm_check)
        {
            rocblas_error_1 = norm_check_general<T>('F', 1, M, abs_incx, hx_gold, hx_1);
        }
    }

    if(arg.timing)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;

        for(int iter = 0; iter < number_cold_calls; iter++)
        {
            rocblas_tbmv_fn(handle, uplo, transA, diag, M, K, dA, lda, dx, incx);
        }

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int iter = 0; iter < number_hot_calls; iter++)
        {
            rocblas_tbmv_fn(handle, uplo, transA, diag, M, K, dA, lda, dx, incx);
        }

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        ArgumentModel<e_uplo, e_transA, e_diag, e_M, e_K, e_lda, e_incx>{}.log_args<T>(
            rocblas_cout,
            arg,
            gpu_time_used,
            tbmv_gflop_count<T>(M, K),
            tbmv_gbyte_count<T>(M, K),
            cpu_time_used,
            rocblas_error_1);
    }
}
