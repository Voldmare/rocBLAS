/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "cblas_interface.hpp"
#include "norm.hpp"
#include "rocblas.hpp"
#include "rocblas_init.hpp"
#include "rocblas_math.hpp"
#include "rocblas_random.hpp"
#include "rocblas_test.hpp"
#include "rocblas_vector.hpp"
#include "unit.hpp"
#include "utility.hpp"

template <typename T>
void testing_rotm_strided_batched_bad_arg(const Arguments& arg)
{
    const bool FORTRAN = arg.fortran;
    auto       rocblas_rotm_strided_batched_fn
        = FORTRAN ? rocblas_rotm_strided_batched<T, true> : rocblas_rotm_strided_batched<T, false>;

    rocblas_int         N            = 100;
    rocblas_int         incx         = 1;
    rocblas_stride      stride_x     = 1;
    rocblas_int         incy         = 1;
    rocblas_stride      stride_y     = 1;
    rocblas_stride      stride_param = 1;
    rocblas_int         batch_count  = 5;
    static const size_t safe_size    = 100;

    rocblas_local_handle handle{arg};
    device_vector<T>     dx(safe_size);
    device_vector<T>     dy(safe_size);
    device_vector<T>     dparam(safe_size);
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy.memcheck());
    CHECK_DEVICE_ALLOCATION(dparam.memcheck());

    CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
    EXPECT_ROCBLAS_STATUS(
        (rocblas_rotm_strided_batched_fn(
            nullptr, N, dx, incx, stride_x, dy, incy, stride_y, dparam, stride_param, batch_count)),
        rocblas_status_invalid_handle);
    EXPECT_ROCBLAS_STATUS((rocblas_rotm_strided_batched_fn(handle,
                                                           N,
                                                           nullptr,
                                                           incx,
                                                           stride_x,
                                                           dy,
                                                           incy,
                                                           stride_y,
                                                           dparam,
                                                           stride_param,
                                                           batch_count)),
                          rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS((rocblas_rotm_strided_batched_fn(handle,
                                                           N,
                                                           dx,
                                                           incx,
                                                           stride_x,
                                                           nullptr,
                                                           incy,
                                                           stride_y,
                                                           dparam,
                                                           stride_param,
                                                           batch_count)),
                          rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS(
        (rocblas_rotm_strided_batched_fn(
            handle, N, dx, incx, stride_x, dy, incy, stride_y, nullptr, stride_param, batch_count)),
        rocblas_status_invalid_pointer);
}

template <typename T>
void testing_rotm_strided_batched(const Arguments& arg)
{
    const bool FORTRAN = arg.fortran;
    auto       rocblas_rotm_strided_batched_fn
        = FORTRAN ? rocblas_rotm_strided_batched<T, true> : rocblas_rotm_strided_batched<T, false>;

    rocblas_int N            = arg.N;
    rocblas_int incx         = arg.incx;
    rocblas_int stride_x     = arg.stride_x;
    rocblas_int stride_y     = arg.stride_y;
    rocblas_int stride_param = arg.stride_c;
    rocblas_int incy         = arg.incy;
    rocblas_int batch_count  = arg.batch_count;

    rocblas_local_handle handle{arg};
    double               gpu_time_used, cpu_time_used;
    double norm_error_host_x = 0.0, norm_error_host_y = 0.0, norm_error_device_x = 0.0,
           norm_error_device_y = 0.0;
    const T rel_error          = std::numeric_limits<T>::epsilon() * 1000;

    // check to prevent undefined memory allocation error
    if(N <= 0 || batch_count <= 0)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        EXPECT_ROCBLAS_STATUS((rocblas_rotm_strided_batched_fn(handle,
                                                               N,
                                                               nullptr,
                                                               incx,
                                                               stride_x,
                                                               nullptr,
                                                               incy,
                                                               stride_y,
                                                               nullptr,
                                                               stride_param,
                                                               batch_count)),
                              rocblas_status_success);
        return;
    }

    rocblas_int abs_incx   = incx >= 0 ? incx : -incx;
    rocblas_int abs_incy   = incy >= 0 ? incy : -incy;
    size_t      size_x     = N * size_t(abs_incx) + size_t(stride_x) * size_t(batch_count - 1);
    size_t      size_y     = N * size_t(abs_incy) + size_t(stride_y) * size_t(batch_count - 1);
    size_t      size_param = 5 + size_t(stride_param) * size_t(batch_count - 1);

    device_vector<T> dx(size_x);
    device_vector<T> dy(size_y);
    device_vector<T> dparam(size_param);
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy.memcheck());
    CHECK_DEVICE_ALLOCATION(dparam.memcheck());

    // Initial Data on CPU
    host_vector<T> hx(size_x);
    host_vector<T> hy(size_y);
    host_vector<T> hdata(4 * batch_count);
    host_vector<T> hparam(size_param);

    // Initialize data on host memory
    rocblas_init_vector(
        hx, arg, N, abs_incx, stride_x, batch_count, rocblas_client_alpha_sets_nan, true);
    rocblas_init_vector(
        hy, arg, N, abs_incy, stride_y, batch_count, rocblas_client_alpha_sets_nan, false);
    rocblas_init_vector(hdata, arg, 4, 1, 4, batch_count, rocblas_client_alpha_sets_nan, false);

    for(int b = 0; b < batch_count; b++)
    {
        T* hparam_ptr = hparam.data() + b * stride_param;

        // generating simply one set of hparam which will not be appropriate for testing
        // that it zeros out the second element of the rotm vector parameter
        memset(hparam_ptr, 0, 5 * sizeof(T));

        cblas_rotmg<T>(
            hdata + b * 4, hdata + b * 4 + 1, hdata + b * 4 + 2, hdata + b * 4 + 3, hparam_ptr);
    }

    constexpr int FLAG_COUNT        = 4;
    const T       FLAGS[FLAG_COUNT] = {-1, 0, 1, -2};

    for(int i = 0; i < FLAG_COUNT; i++)
    {
        for(int b = 0; b < batch_count; b++)
            (hparam + b * stride_param)[0] = FLAGS[i];

        // CPU BLAS reference data
        host_vector<T> cx = hx;
        host_vector<T> cy = hy;
        cpu_time_used     = get_time_us_no_sync();
        for(int b = 0; b < batch_count; b++)
        {
            cblas_rotm<T>(
                N, cx + b * stride_x, incx, cy + b * stride_y, incy, hparam + b * stride_param);
        }
        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        if(arg.unit_check || arg.norm_check)
        {
            // Test rocblas_pointer_mode_host
            // TODO: THIS IS NO LONGER SUPPORTED
            // {
            //     CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
            //     CHECK_HIP_ERROR(hipMemcpy(dx, hx, sizeof(T) * size_x, hipMemcpyHostToDevice));
            //     CHECK_HIP_ERROR(hipMemcpy(dy, hy, sizeof(T) * size_y, hipMemcpyHostToDevice));
            //     CHECK_ROCBLAS_ERROR((rocblas_rotm_strided_batched_fn(
            //         handle, N, dx, incx, stride_x, dy, incy, stride_y, hparam, batch_count)));
            //     host_vector<T> rx(size_x);
            //     host_vector<T> ry(size_y);
            //     CHECK_HIP_ERROR(hipMemcpy(rx, dx, sizeof(T) * size_x, hipMemcpyDeviceToHost));
            //     CHECK_HIP_ERROR(hipMemcpy(ry, dy, sizeof(T) * size_y, hipMemcpyDeviceToHost));
            //     if(arg.unit_check)
            //     {
            //         T rel_error = std::numeric_limits<T>::epsilon() * 1000;
            //         near_check_general<T,T>(1, N, batch_count, incx, stride_x, cx, rx, rel_error);
            //         near_check_general<T,T>(1, N, batch_count, incy, stride_y, cy, ry, rel_error);
            //     }
            //     if(arg.norm_check)
            //     {
            //         norm_error_host_x
            //             = norm_check_general<T>('F', 1, N, incx, stride_x, batch_count, cx, rx);
            //         norm_error_host_y
            //             = norm_check_general<T>('F', 1, N, incy, stride_x, batch_count, cy, ry);
            //     }
            // }

            // Test rocblas_pointer_mode_device
            {
                CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
                CHECK_HIP_ERROR(hipMemcpy(dx, hx, sizeof(T) * size_x, hipMemcpyHostToDevice));
                CHECK_HIP_ERROR(hipMemcpy(dy, hy, sizeof(T) * size_y, hipMemcpyHostToDevice));
                CHECK_HIP_ERROR(
                    hipMemcpy(dparam, hparam, sizeof(T) * size_param, hipMemcpyHostToDevice));
                CHECK_ROCBLAS_ERROR((rocblas_rotm_strided_batched_fn(handle,
                                                                     N,
                                                                     dx,
                                                                     incx,
                                                                     stride_x,
                                                                     dy,
                                                                     incy,
                                                                     stride_y,
                                                                     dparam,
                                                                     stride_param,
                                                                     batch_count)));
                host_vector<T> rx(size_x);
                host_vector<T> ry(size_y);
                CHECK_HIP_ERROR(hipMemcpy(rx, dx, sizeof(T) * size_x, hipMemcpyDeviceToHost));
                CHECK_HIP_ERROR(hipMemcpy(ry, dy, sizeof(T) * size_y, hipMemcpyDeviceToHost));

                if(arg.unit_check)
                {
                    near_check_general<T>(1, N, abs_incx, stride_x, cx, rx, batch_count, rel_error);
                    near_check_general<T>(1, N, abs_incy, stride_y, cy, ry, batch_count, rel_error);
                }

                if(arg.norm_check)
                {
                    norm_error_device_x += norm_check_general<T>(
                        'F', 1, N, abs_incx, stride_x, cx, rx, batch_count);
                    norm_error_device_y += norm_check_general<T>(
                        'F', 1, N, abs_incy, stride_y, cy, ry, batch_count);
                }
            }
        }
    }
    if(arg.timing)
    {
        // Initializing flag value to -1 for all the batches of hparam
        for(int b = 0; b < batch_count; b++)
        {
            (hparam + b * stride_param)[0] = FLAGS[0];
        }

        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_HIP_ERROR(hipMemcpy(dx, hx, sizeof(T) * size_x, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(dy, hy, sizeof(T) * size_y, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(dparam, hparam, sizeof(T) * size_param, hipMemcpyHostToDevice));

        for(int iter = 0; iter < number_cold_calls; iter++)
        {
            rocblas_rotm_strided_batched_fn(handle,
                                            N,
                                            dx,
                                            incx,
                                            stride_x,
                                            dy,
                                            incy,
                                            stride_y,
                                            dparam,
                                            stride_param,
                                            batch_count);
        }
        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds
        for(int iter = 0; iter < number_hot_calls; iter++)
        {
            rocblas_rotm_strided_batched_fn(handle,
                                            N,
                                            dx,
                                            incx,
                                            stride_x,
                                            dy,
                                            incy,
                                            stride_y,
                                            dparam,
                                            stride_param,
                                            batch_count);
        }
        gpu_time_used = (get_time_us_sync(stream) - gpu_time_used);

        ArgumentModel<e_N, e_incx, e_incy, e_stride_x, e_stride_y, e_batch_count>{}.log_args<T>(
            rocblas_cout,
            arg,
            gpu_time_used,
            rotm_gflop_count<T>(N, (hparam + 0 * stride_param)[0]),
            rotm_gbyte_count<T>(N, (hparam + 0 * stride_param)[0]),
            cpu_time_used,
            norm_error_device_x,
            norm_error_device_y);
    }
}
