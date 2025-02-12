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

template <typename T, typename U = T, typename V = T>
void testing_rot_batched_bad_arg(const Arguments& arg)
{
    auto rocblas_rot_batched_fn
        = arg.fortran ? rocblas_rot_batched<T, U, V, true> : rocblas_rot_batched<T, U, V, false>;

    rocblas_int N           = 100;
    rocblas_int incx        = 1;
    rocblas_int incy        = 1;
    rocblas_int batch_count = 5;

    rocblas_local_handle   handle{arg};
    device_batch_vector<T> dx(N, incx, batch_count);
    device_batch_vector<T> dy(N, incy, batch_count);
    device_vector<U>       dc(1);
    device_vector<V>       ds(1);
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy.memcheck());
    CHECK_DEVICE_ALLOCATION(dc.memcheck());
    CHECK_DEVICE_ALLOCATION(ds.memcheck());

    EXPECT_ROCBLAS_STATUS(
        (rocblas_rot_batched_fn(
            nullptr, N, dx.ptr_on_device(), incx, dy.ptr_on_device(), incy, dc, ds, batch_count)),
        rocblas_status_invalid_handle);
    EXPECT_ROCBLAS_STATUS(
        (rocblas_rot_batched_fn(
            handle, N, nullptr, incx, dy.ptr_on_device(), incy, dc, ds, batch_count)),
        rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS(
        (rocblas_rot_batched_fn(
            handle, N, dx.ptr_on_device(), incx, nullptr, incy, dc, ds, batch_count)),
        rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS((rocblas_rot_batched_fn(handle,
                                                  N,
                                                  dx.ptr_on_device(),
                                                  incx,
                                                  dy.ptr_on_device(),
                                                  incy,
                                                  nullptr,
                                                  ds,
                                                  batch_count)),
                          rocblas_status_invalid_pointer);
    EXPECT_ROCBLAS_STATUS((rocblas_rot_batched_fn(handle,
                                                  N,
                                                  dx.ptr_on_device(),
                                                  incx,
                                                  dy.ptr_on_device(),
                                                  incy,
                                                  dc,
                                                  nullptr,
                                                  batch_count)),
                          rocblas_status_invalid_pointer);
}

template <typename T, typename U = T, typename V = T>
void testing_rot_batched(const Arguments& arg)
{
    auto rocblas_rot_batched_fn
        = arg.fortran ? rocblas_rot_batched<T, U, V, true> : rocblas_rot_batched<T, U, V, false>;

    rocblas_int N           = arg.N;
    rocblas_int incx        = arg.incx;
    rocblas_int incy        = arg.incy;
    rocblas_int batch_count = arg.batch_count;

    rocblas_local_handle handle{arg};
    double               gpu_time_used, cpu_time_used;
    double norm_error_host_x = 0.0, norm_error_host_y = 0.0, norm_error_device_x = 0.0,
           norm_error_device_y = 0.0;

    // check to prevent undefined memory allocation error
    if(N <= 0 || batch_count <= 0)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_ROCBLAS_ERROR((rocblas_rot_batched_fn(
            handle, N, nullptr, incx, nullptr, incy, nullptr, nullptr, batch_count)));
        return;
    }

    rocblas_int abs_incx = incx >= 0 ? incx : -incx;
    rocblas_int abs_incy = incy >= 0 ? incy : -incy;
    size_t      size_x   = N * size_t(abs_incx);
    size_t      size_y   = N * size_t(abs_incy);

    device_batch_vector<T> dx(N, incx, batch_count);
    device_batch_vector<T> dy(N, incy, batch_count);
    device_vector<U>       dc(1);
    device_vector<V>       ds(1);
    CHECK_DEVICE_ALLOCATION(dx.memcheck());
    CHECK_DEVICE_ALLOCATION(dy.memcheck());
    CHECK_DEVICE_ALLOCATION(dc.memcheck());
    CHECK_DEVICE_ALLOCATION(ds.memcheck());

    // Initial Data on CPU
    host_batch_vector<T> hx(N, incx, batch_count);
    host_batch_vector<T> hy(N, incy, batch_count);
    host_vector<U>       hc(1);
    host_vector<V>       hs(1);

    // Initialize data on host memory
    rocblas_init_vector(hx, arg, rocblas_client_alpha_sets_nan, true);
    rocblas_init_vector(hy, arg, rocblas_client_alpha_sets_nan, false);
    rocblas_init_vector(hc, arg, 1, 1, 0, 1, rocblas_client_alpha_sets_nan, false);
    rocblas_init_vector(hs, arg, 1, 1, 0, 1, rocblas_client_alpha_sets_nan, false);

    // CPU BLAS reference data
    host_batch_vector<T> cx(N, incx, batch_count);
    host_batch_vector<T> cy(N, incy, batch_count);
    cx.copy_from(hx);
    cy.copy_from(hy);

    // cblas_rotg<T, U>(cx, cy, hc, hs);
    // cx[0] = hx[0];
    // cy[0] = hy[0];
    cpu_time_used = get_time_us_no_sync();
    for(int b = 0; b < batch_count; b++)
    {
        cblas_rot<T, T, U, V>(N, cx[b], incx, cy[b], incy, hc, hs);
    }
    cpu_time_used = get_time_us_no_sync() - cpu_time_used;

    if(arg.unit_check || arg.norm_check)
    {
        // Test rocblas_pointer_mode_host
        {
            CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
            CHECK_HIP_ERROR(dx.transfer_from(hx));
            CHECK_HIP_ERROR(dy.transfer_from(hy));

            CHECK_ROCBLAS_ERROR((rocblas_rot_batched_fn(handle,
                                                        N,
                                                        dx.ptr_on_device(),
                                                        incx,
                                                        dy.ptr_on_device(),
                                                        incy,
                                                        hc,
                                                        hs,
                                                        batch_count)));

            host_batch_vector<T> rx(N, incx, batch_count);
            host_batch_vector<T> ry(N, incy, batch_count);

            CHECK_HIP_ERROR(rx.transfer_from(dx));
            CHECK_HIP_ERROR(ry.transfer_from(dy));

            if(arg.unit_check)
            {
                unit_check_general<T>(1, N, abs_incx, cx, rx, batch_count);
                unit_check_general<T>(1, N, abs_incy, cy, ry, batch_count);
            }
            if(arg.norm_check)
            {
                norm_error_host_x = norm_check_general<T>('F', 1, N, abs_incx, cx, rx, batch_count);
                norm_error_host_y = norm_check_general<T>('F', 1, N, abs_incy, cy, ry, batch_count);
            }
        }

        // Test rocblas_pointer_mode_device
        {
            CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
            CHECK_HIP_ERROR(dx.transfer_from(hx));
            CHECK_HIP_ERROR(dy.transfer_from(hy));

            CHECK_HIP_ERROR(dc.transfer_from(hc));
            CHECK_HIP_ERROR(ds.transfer_from(hs));

            CHECK_ROCBLAS_ERROR((rocblas_rot_batched_fn(handle,
                                                        N,
                                                        dx.ptr_on_device(),
                                                        incx,
                                                        dy.ptr_on_device(),
                                                        incy,
                                                        dc,
                                                        ds,
                                                        batch_count)));

            host_batch_vector<T> rx(N, incx, batch_count);
            host_batch_vector<T> ry(N, incy, batch_count);
            CHECK_HIP_ERROR(rx.transfer_from(dx));
            CHECK_HIP_ERROR(ry.transfer_from(dy));

            if(arg.unit_check)
            {
                unit_check_general<T>(1, N, abs_incx, cx, rx, batch_count);
                unit_check_general<T>(1, N, abs_incy, cy, ry, batch_count);
            }
            if(arg.norm_check)
            {
                norm_error_device_x
                    = norm_check_general<T>('F', 1, N, abs_incx, cx, rx, batch_count);
                norm_error_device_y
                    = norm_check_general<T>('F', 1, N, abs_incy, cy, ry, batch_count);
            }
        }
    }

    if(arg.timing)
    {
        int number_cold_calls = arg.cold_iters;
        int number_hot_calls  = arg.iters;
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));
        CHECK_HIP_ERROR(dx.transfer_from(hx));
        CHECK_HIP_ERROR(dy.transfer_from(hy));
        CHECK_HIP_ERROR(dc.transfer_from(hc));
        CHECK_HIP_ERROR(ds.transfer_from(hs));

        for(int iter = 0; iter < number_cold_calls; iter++)
        {
            rocblas_rot_batched_fn(
                handle, N, dx.ptr_on_device(), incx, dy.ptr_on_device(), incy, dc, ds, batch_count);
        }
        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds
        for(int iter = 0; iter < number_hot_calls; iter++)
        {
            rocblas_rot_batched_fn(
                handle, N, dx.ptr_on_device(), incx, dy.ptr_on_device(), incy, dc, ds, batch_count);
        }
        gpu_time_used = (get_time_us_sync(stream) - gpu_time_used);

        ArgumentModel<e_N, e_incx, e_incy, e_batch_count>{}.log_args<T>(
            rocblas_cout,
            arg,
            gpu_time_used,
            rot_gflop_count<T, T, U, V>(N),
            rot_gbyte_count<T>(N),
            cpu_time_used,
            norm_error_host_x,
            norm_error_device_x,
            norm_error_host_y,
            norm_error_device_y);
    }
}
