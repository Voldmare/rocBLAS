/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 *
 * ************************************************************************ */

#pragma once

#include "bytes.hpp"
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

template <typename T>
void testing_gemv_strided_batched_bad_arg(const Arguments& arg)
{
    auto rocblas_gemv_strided_batched_fn = arg.fortran ? rocblas_gemv_strided_batched<T, true>
                                                       : rocblas_gemv_strided_batched<T, false>;

    const rocblas_int M           = 100;
    const rocblas_int N           = 100;
    const rocblas_int lda         = 100;
    const rocblas_int incx        = 1;
    const rocblas_int incy        = 1;
    const T           alpha       = 2.0;
    const T           beta        = 0.5;
    const T           zero        = 0.0;
    const T           one         = 1.0;
    const rocblas_int stride_a    = 10000;
    const rocblas_int stride_x    = 100;
    const rocblas_int stride_y    = 100;
    const rocblas_int batch_count = 5;

    const rocblas_operation transA = rocblas_operation_none;

    rocblas_local_handle handle{arg};

    size_t size_A = lda * static_cast<size_t>(N);

    // allocate memory on device
    device_strided_batch_vector<T> dA(size_A, 1, stride_a, batch_count);
    device_strided_batch_vector<T> dx(N, incx, stride_x, batch_count);
    device_strided_batch_vector<T> dy(M, incy, stride_y, batch_count);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy.memcheck());

    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          &alpha,
                                                          nullptr,
                                                          lda,
                                                          stride_a,
                                                          dx,
                                                          incx,
                                                          stride_x,
                                                          &beta,
                                                          dy,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          &alpha,
                                                          dA,
                                                          lda,
                                                          stride_a,
                                                          nullptr,
                                                          incx,
                                                          stride_x,
                                                          &beta,
                                                          dy,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          &alpha,
                                                          dA,
                                                          lda,
                                                          stride_a,
                                                          dx,
                                                          incx,
                                                          stride_x,
                                                          &beta,
                                                          nullptr,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          nullptr,
                                                          dA,
                                                          lda,
                                                          stride_a,
                                                          dx,
                                                          incx,
                                                          stride_x,
                                                          &beta,
                                                          dy,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          &alpha,
                                                          dA,
                                                          lda,
                                                          stride_a,
                                                          dx,
                                                          incx,
                                                          stride_x,
                                                          nullptr,
                                                          dy,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_invalid_pointer);

    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(nullptr,
                                                          transA,
                                                          M,
                                                          N,
                                                          &alpha,
                                                          dA,
                                                          lda,
                                                          stride_a,
                                                          dx,
                                                          incx,
                                                          stride_x,
                                                          &beta,
                                                          dy,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_invalid_handle);

    // When batch_count==0, all pointers may be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          nullptr,
                                                          nullptr,
                                                          lda,
                                                          stride_a,
                                                          nullptr,
                                                          incx,
                                                          stride_x,
                                                          nullptr,
                                                          nullptr,
                                                          incy,
                                                          stride_y,
                                                          0),
                          rocblas_status_success);

    // When M==0, all pointers may be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          0,
                                                          N,
                                                          nullptr,
                                                          nullptr,
                                                          lda,
                                                          stride_a,
                                                          nullptr,
                                                          incx,
                                                          stride_x,
                                                          nullptr,
                                                          nullptr,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_success);

    // When N==0, all pointers may be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          0,
                                                          nullptr,
                                                          nullptr,
                                                          lda,
                                                          stride_a,
                                                          nullptr,
                                                          incx,
                                                          stride_x,
                                                          nullptr,
                                                          nullptr,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_success);

    // When alpha==0, A and x may be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          &zero,
                                                          nullptr,
                                                          lda,
                                                          stride_a,
                                                          nullptr,
                                                          incx,
                                                          stride_x,
                                                          &beta,
                                                          dy,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_success);

    // When alpha==0 && beta==1, A, x and y may be nullptr without error
    EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                          transA,
                                                          M,
                                                          N,
                                                          &zero,
                                                          nullptr,
                                                          lda,
                                                          stride_a,
                                                          nullptr,
                                                          incx,
                                                          stride_x,
                                                          &one,
                                                          nullptr,
                                                          incy,
                                                          stride_y,
                                                          batch_count),
                          rocblas_status_success);
}

template <typename T>
void testing_gemv_strided_batched(const Arguments& arg)
{
    auto rocblas_gemv_strided_batched_fn = arg.fortran ? rocblas_gemv_strided_batched<T, true>
                                                       : rocblas_gemv_strided_batched<T, false>;

    rocblas_int       M           = arg.M;
    rocblas_int       N           = arg.N;
    rocblas_int       lda         = arg.lda;
    rocblas_int       incx        = arg.incx;
    rocblas_int       incy        = arg.incy;
    T                 h_alpha     = arg.get_alpha<T>();
    T                 h_beta      = arg.get_beta<T>();
    rocblas_operation transA      = char2rocblas_operation(arg.transA);
    rocblas_int       stride_a    = arg.stride_a;
    rocblas_int       stride_x    = arg.stride_x;
    rocblas_int       stride_y    = arg.stride_y;
    rocblas_int       batch_count = arg.batch_count;

    rocblas_local_handle handle{arg};
    size_t               size_A = lda * static_cast<size_t>(N);
    size_t               size_x, dim_x, abs_incx;
    size_t               size_y, dim_y, abs_incy;

    if(transA == rocblas_operation_none)
    {
        dim_x = N;
        dim_y = M;
    }
    else
    {
        dim_x = M;
        dim_y = N;
    }

    abs_incx = incx >= 0 ? incx : -incx;
    abs_incy = incy >= 0 ? incy : -incy;

    size_x = dim_x * abs_incx;
    size_y = dim_y * abs_incy;

    // argument sanity check before allocating invalid memory
    bool invalid_size = M < 0 || N < 0 || lda < M || lda < 1 || !incx || !incy || batch_count < 0;
    if(invalid_size || !M || !N || !batch_count)
    {
        EXPECT_ROCBLAS_STATUS(rocblas_gemv_strided_batched_fn(handle,
                                                              transA,
                                                              M,
                                                              N,
                                                              nullptr,
                                                              nullptr,
                                                              lda,
                                                              stride_a,
                                                              nullptr,
                                                              incx,
                                                              stride_x,
                                                              nullptr,
                                                              nullptr,
                                                              incy,
                                                              stride_y,
                                                              batch_count),
                              invalid_size ? rocblas_status_invalid_size : rocblas_status_success);
        return;
    }

    size_A = size_A + static_cast<size_t>(stride_a) * static_cast<size_t>(batch_count - 1);
    size_x = size_x + static_cast<size_t>(stride_x) * static_cast<size_t>(batch_count - 1);
    size_y = size_y + static_cast<size_t>(stride_y) * static_cast<size_t>(batch_count - 1);

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<T> hA(size_A);
    host_vector<T> hx(size_x);
    host_vector<T> hy_1(size_y);
    host_vector<T> hy_2(size_y);
    host_vector<T> hy_gold(size_y);

    device_vector<T> dA(size_A);
    device_vector<T> dx(size_x);
    device_vector<T> dy_1(size_y);
    device_vector<T> dy_2(size_y);
    device_vector<T> d_alpha(1);
    device_vector<T> d_beta(1);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy_1.memcheck());
    CHECK_DEVICE_ALLOCATION(dy_2.memcheck());
    CHECK_DEVICE_ALLOCATION(d_alpha.memcheck());
    CHECK_DEVICE_ALLOCATION(d_beta.memcheck());

    // Initialize data on host memory
    rocblas_init_matrix(hA,
                        arg,
                        M,
                        N,
                        lda,
                        stride_a,
                        batch_count,
                        rocblas_client_alpha_sets_nan,
                        rocblas_client_general_matrix,
                        true);
    rocblas_init_vector(hx,
                        arg,
                        dim_x,
                        abs_incx,
                        stride_x,
                        batch_count,
                        rocblas_client_alpha_sets_nan,
                        false,
                        true);
    rocblas_init_vector(
        hy_1, arg, dim_y, abs_incy, stride_y, batch_count, rocblas_client_beta_sets_nan);

    // copy vector is easy in STL; hy_gold = hy_1: save a copy in hy_gold which will be output of
    // CPU BLAS
    hy_gold = hy_1;
    hy_2    = hy_1;

    // copy data from CPU to device
    CHECK_HIP_ERROR(hipMemcpy(dA, hA, sizeof(T) * size_A, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dx, hx, sizeof(T) * size_x, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dy_1, hy_1, sizeof(T) * size_y, hipMemcpyHostToDevice));

    double gpu_time_used, cpu_time_used;
    double rocblas_error_1;
    double rocblas_error_2;

    /* =====================================================================
           ROCBLAS
    =================================================================== */
    if(arg.unit_check || arg.norm_check)
    {
        CHECK_HIP_ERROR(hipMemcpy(dy_2, hy_2, sizeof(T) * size_y, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(d_alpha, &h_alpha, sizeof(T), hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(d_beta, &h_beta, sizeof(T), hipMemcpyHostToDevice));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        CHECK_ROCBLAS_ERROR(rocblas_gemv_strided_batched_fn(handle,
                                                            transA,
                                                            M,
                                                            N,
                                                            &h_alpha,
                                                            dA,
                                                            lda,
                                                            stride_a,
                                                            dx,
                                                            incx,
                                                            stride_x,
                                                            &h_beta,
                                                            dy_1,
                                                            incy,
                                                            stride_y,
                                                            batch_count));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_ROCBLAS_ERROR(rocblas_gemv_strided_batched_fn(handle,
                                                            transA,
                                                            M,
                                                            N,
                                                            d_alpha,
                                                            dA,
                                                            lda,
                                                            stride_a,
                                                            dx,
                                                            incx,
                                                            stride_x,
                                                            d_beta,
                                                            dy_2,
                                                            incy,
                                                            stride_y,
                                                            batch_count));

        // CPU BLAS
        cpu_time_used = get_time_us_no_sync();
        for(int b = 0; b < batch_count; ++b)
        {
            cblas_gemv<T>(transA,
                          M,
                          N,
                          h_alpha,
                          hA + b * stride_a,
                          lda,
                          hx + b * stride_x,
                          incx,
                          h_beta,
                          hy_gold + b * stride_y,
                          incy);
        }
        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        // copy output from device to CPU
        CHECK_HIP_ERROR(hipMemcpy(hy_1, dy_1, sizeof(T) * size_y, hipMemcpyDeviceToHost));
        CHECK_HIP_ERROR(hipMemcpy(hy_2, dy_2, sizeof(T) * size_y, hipMemcpyDeviceToHost));

        if(arg.unit_check)
        {
            unit_check_general<T>(1, dim_y, abs_incy, stride_y, hy_gold, hy_1, batch_count);
            unit_check_general<T>(1, dim_y, abs_incy, stride_y, hy_gold, hy_2, batch_count);
        }

        if(arg.norm_check)
        {
            rocblas_error_1 = norm_check_general<T>(
                'F', 1, dim_y, abs_incy, stride_y, hy_gold, hy_1, batch_count);
            rocblas_error_2 = norm_check_general<T>(
                'F', 1, dim_y, abs_incy, stride_y, hy_gold, hy_2, batch_count);
        }
    }

    if(arg.timing)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        for(int iter = 0; iter < number_cold_calls; iter++)
        {
            rocblas_gemv_strided_batched_fn(handle,
                                            transA,
                                            M,
                                            N,
                                            &h_alpha,
                                            dA,
                                            lda,
                                            stride_a,
                                            dx,
                                            incx,
                                            stride_x,
                                            &h_beta,
                                            dy_1,
                                            incy,
                                            stride_y,
                                            batch_count);
        }

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int iter = 0; iter < number_hot_calls; iter++)
        {
            rocblas_gemv_strided_batched_fn(handle,
                                            transA,
                                            M,
                                            N,
                                            &h_alpha,
                                            dA,
                                            lda,
                                            stride_a,
                                            dx,
                                            incx,
                                            stride_x,
                                            &h_beta,
                                            dy_1,
                                            incy,
                                            stride_y,
                                            batch_count);
        }

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        ArgumentModel<e_transA,
                      e_M,
                      e_N,
                      e_alpha,
                      e_lda,
                      e_stride_a,
                      e_incx,
                      e_stride_x,
                      e_beta,
                      e_incy,
                      e_stride_y,
                      e_batch_count>{}
            .log_args<T>(rocblas_cout,
                         arg,
                         gpu_time_used,
                         gemv_gflop_count<T>(transA, M, N),
                         gemv_gbyte_count<T>(transA, M, N),
                         cpu_time_used,
                         rocblas_error_1,
                         rocblas_error_2);
    }
}
