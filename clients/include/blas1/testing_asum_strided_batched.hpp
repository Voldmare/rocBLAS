/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "bytes.hpp"
#include "cblas_interface.hpp"
#include "flops.hpp"
#include "near.hpp"
#include "rocblas.hpp"
#include "rocblas_init.hpp"
#include "rocblas_math.hpp"
#include "rocblas_random.hpp"
#include "rocblas_test.hpp"
#include "rocblas_vector.hpp"
#include "unit.hpp"
#include "utility.hpp"

template <typename T>
void testing_asum_strided_batched_bad_arg(const Arguments& arg)
{
    const bool FORTRAN = arg.fortran;
    auto       rocblas_asum_strided_batched_fn
        = FORTRAN ? rocblas_asum_strided_batched<T, true> : rocblas_asum_strided_batched<T, false>;

    rocblas_int    N           = 100;
    rocblas_int    incx        = 1;
    rocblas_stride stridex     = N;
    rocblas_int    batch_count = 5;
    real_t<T>      h_rocblas_result[1];

    device_strided_batch_vector<T> dx(N, incx, stridex, batch_count);
    CHECK_DEVICE_ALLOCATION(dx.memcheck());

    rocblas_local_handle handle{arg};
    CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

    EXPECT_ROCBLAS_STATUS(rocblas_asum_strided_batched_fn(
                              handle, N, nullptr, incx, stridex, batch_count, h_rocblas_result),
                          rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS(
        rocblas_asum_strided_batched_fn(handle, N, dx, incx, stridex, batch_count, nullptr),
        rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS(rocblas_asum_strided_batched_fn(
                              nullptr, N, dx, incx, stridex, batch_count, h_rocblas_result),
                          rocblas_status_invalid_handle);
};

template <typename T>
void testing_asum_strided_batched(const Arguments& arg)
{
    const bool FORTRAN = arg.fortran;
    auto       rocblas_asum_strided_batched_fn
        = FORTRAN ? rocblas_asum_strided_batched<T, true> : rocblas_asum_strided_batched<T, false>;

    rocblas_int    N           = arg.N;
    rocblas_int    incx        = arg.incx;
    rocblas_stride stridex     = arg.stride_x;
    rocblas_int    batch_count = arg.batch_count;

    double rocblas_error_1;
    double rocblas_error_2;

    rocblas_local_handle handle{arg};

    // check to prevent undefined memory allocation error
    if(N <= 0 || incx <= 0 || batch_count <= 0)
    {
        host_vector<real_t<T>> hr1(std::max(1, std::abs(batch_count)));
        host_vector<real_t<T>> hr2(std::max(1, std::abs(batch_count)));
        host_vector<real_t<T>> result_0(std::max(1, std::abs(batch_count)));
        CHECK_HIP_ERROR(hr1.memcheck());
        CHECK_HIP_ERROR(hr2.memcheck());
        CHECK_HIP_ERROR(result_0.memcheck());

        device_vector<real_t<T>> dr(std::max(1, std::abs(batch_count)));
        CHECK_DEVICE_ALLOCATION(dr.memcheck());

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        EXPECT_ROCBLAS_STATUS(
            rocblas_asum_strided_batched_fn(handle, N, nullptr, incx, stridex, batch_count, dr),
            rocblas_status_success);

        CHECK_HIP_ERROR(hr1.transfer_from(dr));

        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        EXPECT_ROCBLAS_STATUS(
            rocblas_asum_strided_batched_fn(handle, N, nullptr, incx, stridex, batch_count, hr2),
            rocblas_status_success);

        if(batch_count > 0)
        {
            unit_check_general<real_t<T>, real_t<T>>(1, batch_count, 1, result_0, hr1);
            unit_check_general<real_t<T>, real_t<T>>(1, batch_count, 1, result_0, hr2);
        }

        return;
    }

    // allocate memory on device
    // Naming: dx is in GPU (device) memory. hx is in CPU (host) memory, plz follow this practice

    host_strided_batch_vector<T> hx(N, incx, stridex, batch_count);
    CHECK_HIP_ERROR(hx.memcheck());
    device_strided_batch_vector<T> dx(N, incx, stridex, batch_count);
    CHECK_DEVICE_ALLOCATION(dx.memcheck());

    device_vector<real_t<T>> dr(batch_count);
    CHECK_DEVICE_ALLOCATION(dr.memcheck());
    host_vector<real_t<T>> hr1(batch_count);
    CHECK_HIP_ERROR(hr1.memcheck());
    host_vector<real_t<T>> hr(batch_count);
    CHECK_HIP_ERROR(hr.memcheck());

    //
    // Initialize the host vector.
    //
    rocblas_init_vector(hx, arg, rocblas_client_alpha_sets_nan, true);

    //
    // copy data from CPU to device, does not work for incx != 1
    //
    CHECK_HIP_ERROR(dx.transfer_from(hx));

    double gpu_time_used, cpu_time_used;

    if(arg.unit_check || arg.norm_check)
    {
        // GPU BLAS rocblas_pointer_mode_host
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
        CHECK_ROCBLAS_ERROR(
            rocblas_asum_strided_batched_fn(handle, N, dx, incx, stridex, batch_count, hr1));

        // GPU BgdLAS rocblas_pointer_mode_device
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_ROCBLAS_ERROR(
            rocblas_asum_strided_batched_fn(handle, N, dx, incx, stridex, batch_count, dr));

        CHECK_HIP_ERROR(hr.transfer_from(dr));

        // CPU BLAS
        real_t<T> cpu_result[batch_count];

        cpu_time_used = get_time_us_no_sync();
        for(int i = 0; i < batch_count; i++)
        {
            cblas_asum<T>(N, hx + i * stridex, incx, cpu_result + i);
        }
        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        if(arg.unit_check)
        {
            unit_check_general<real_t<T>, real_t<T>>(batch_count, 1, 1, cpu_result, hr1);
            unit_check_general<real_t<T>, real_t<T>>(batch_count, 1, 1, cpu_result, hr);
        }

        if(arg.norm_check)
        {
            rocblas_cout << "cpu=" << std::scientific << cpu_result[0]
                         << ", gpu_host_ptr=" << hr1[0] << ", gpu_dev_ptr=" << hr[0] << std::endl;

            rocblas_error_1 = std::abs((cpu_result[0] - hr1[0]) / cpu_result[0]);
            rocblas_error_2 = std::abs((cpu_result[0] - hr[0]) / cpu_result[0]);
        }
    }

    if(arg.timing)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));

        for(int iter = 0; iter < number_cold_calls; iter++)
        {
            rocblas_asum_strided_batched_fn(handle, N, dx, incx, stridex, batch_count, dr);
        }

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds

        for(int iter = 0; iter < number_hot_calls; iter++)
        {
            rocblas_asum_strided_batched_fn(handle, N, dx, incx, stridex, batch_count, dr);
        }

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        ArgumentModel<e_N, e_incx, e_stride_x, e_batch_count>{}.log_args<T>(rocblas_cout,
                                                                            arg,
                                                                            gpu_time_used,
                                                                            asum_gflop_count<T>(N),
                                                                            asum_gbyte_count<T>(N),
                                                                            cpu_time_used,
                                                                            rocblas_error_1,
                                                                            rocblas_error_2);
    }
}
