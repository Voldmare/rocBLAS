/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "cblas_interface.hpp"
#include "flops.hpp"
#include "near.hpp"
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
void testing_trtri_batched(const Arguments& arg)
{
    auto rocblas_trtri_batched_fn
        = arg.fortran ? rocblas_trtri_batched<T, true> : rocblas_trtri_batched<T, false>;

    rocblas_int N           = arg.N;
    rocblas_int lda         = arg.lda;
    rocblas_int batch_count = arg.batch_count;

    size_t size_A = size_t(lda) * N;

    char char_uplo = arg.uplo;
    char char_diag = arg.diag;

    // char_uplo = 'U';
    rocblas_fill     uplo = char2rocblas_fill(char_uplo);
    rocblas_diagonal diag = char2rocblas_diagonal(char_diag);

    rocblas_local_handle handle{arg};

    // argument sanity check, quick return if input parameters are invalid before allocating invalid
    bool invalid_size = N < 0 || lda < N || batch_count < 0;
    if(invalid_size || batch_count == 0)
    {
        EXPECT_ROCBLAS_STATUS(rocblas_trtri_batched_fn(
                                  handle, uplo, diag, N, nullptr, lda, nullptr, lda, batch_count),
                              invalid_size ? rocblas_status_invalid_size : rocblas_status_success);
        return;
    }

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_batch_vector<T> hA(size_A, 1, batch_count);
    host_batch_vector<T> hB(size_A, 1, batch_count);
    host_batch_vector<T> hA_2(size_A, 1, batch_count);

    // Initial Data on CPU
    rocblas_seedrand();
    for(size_t b = 0; b < batch_count; b++)
    {
        rocblas_init_symmetric<T>(hA[b], N, lda);
        for(size_t i = 0; i < N; i++)
        {
            for(size_t j = 0; j < N; j++)
            {
                hA[b][i + j * lda] *= 0.01;

                if(j % 2)
                    hA[b][i + j * lda] *= -1;
                if(uplo == rocblas_fill_lower
                   && j > i) // need to explicitly set unsused side to 0 if using it for temp storage
                    hA[b][i + j * lda] = 0.0f;
                else if(uplo == rocblas_fill_upper && j < i)
                    hA[b][i + j * lda] = 0.0f;
                if(i == j)
                {
                    if(diag == rocblas_diagonal_unit)
                        hA[b][i + j * lda] = 1.0; // need to preprocess matrix for clbas_trtri
                    else
                        hA[b][i + j * lda] *= 100.0;
                }
            }
        }
    }

    hB.copy_from(hA);

    double gpu_time_used, cpu_time_used;
    gpu_time_used = cpu_time_used = 0.0;
    double rocblas_error          = 0.0;

    device_batch_vector<T> dA(size_A, 1, batch_count);
    device_batch_vector<T> dinvA(size_A, 1, batch_count);
    CHECK_DEVICE_ALLOCATION(dA.memcheck());
    CHECK_DEVICE_ALLOCATION(dinvA.memcheck());

    // copy data from CPU to device
    CHECK_HIP_ERROR(dA.transfer_from(hA));
    CHECK_HIP_ERROR(dinvA.transfer_from(hA));

    if(!ROCBLAS_REALLOC_ON_DEMAND)
    {
        // Compute size
        CHECK_ROCBLAS_ERROR(rocblas_start_device_memory_size_query(handle));

        CHECK_ALLOC_QUERY(rocblas_trtri_batched_fn(handle,
                                                   uplo,
                                                   diag,
                                                   N,
                                                   dA.ptr_on_device(),
                                                   lda,
                                                   dinvA.ptr_on_device(),
                                                   lda,
                                                   batch_count));

        // Test in place
        CHECK_ALLOC_QUERY(rocblas_trtri_batched_fn(
            handle, uplo, diag, N, dA.ptr_on_device(), lda, dA.ptr_on_device(), lda, batch_count));

        size_t size;
        CHECK_ROCBLAS_ERROR(rocblas_stop_device_memory_size_query(handle, &size));

        // Allocate memory
        CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, size));
    }

    /* =====================================================================
           ROCBLAS
    =================================================================== */
    hipStream_t stream;
    if(arg.timing)
    {
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));
        gpu_time_used = get_time_us_sync(stream); // in microseconds
    }

    CHECK_ROCBLAS_ERROR(rocblas_trtri_batched_fn(
        handle, uplo, diag, N, dA.ptr_on_device(), lda, dinvA.ptr_on_device(), lda, batch_count));

    // Test in place
    CHECK_ROCBLAS_ERROR(rocblas_trtri_batched_fn(
        handle, uplo, diag, N, dA.ptr_on_device(), lda, dA.ptr_on_device(), lda, batch_count));

    if(arg.timing)
    {
        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;
    }

    // copy output from device to CPU
    CHECK_HIP_ERROR(hA.transfer_from(dinvA));
    CHECK_HIP_ERROR(hA_2.transfer_from(dA));

    if(arg.unit_check || arg.norm_check)
    {
        /* =====================================================================
           CPU BLAS
        =================================================================== */
        if(arg.timing)
        {
            cpu_time_used = get_time_us_no_sync();
        }

        for(size_t i = 0; i < batch_count; i++)
        {
            rocblas_int info = cblas_trtri<T>(char_uplo, char_diag, N, hB[i], lda);
            if(info != 0)
            {
#ifdef GOOGLE_TEST
                FAIL() << "error in cblas_trtri";
#else
                rocblas_cerr << "error in cblas_trtri" << std::endl;
#endif
            }
        }
        if(arg.timing)
        {
            cpu_time_used = get_time_us_no_sync() - cpu_time_used;
        }

#if 0
        rocblas_print_matrix(hB[b], hA[b], N, N, lda, 1);
#endif

        if(arg.unit_check)
        {
            const double rel_error = get_epsilon<T>() * 1000;
            near_check_general<T>(N, N, lda, hB, hA, batch_count, rel_error);
            near_check_general<T>(N, N, lda, hB, hA_2, batch_count, rel_error);
        }

        if(arg.norm_check)
        {
            for(size_t i = 0; i < batch_count; i++)
            {
                rocblas_error = fmax(rocblas_error,
                                     norm_check_symmetric<T>('F', char_uplo, N, lda, hB[i], hA[i]));
            }
            rocblas_error = 0.0;
            for(size_t i = 0; i < batch_count; i++)
            {
                rocblas_error = fmax(
                    rocblas_error, norm_check_symmetric<T>('F', char_uplo, N, lda, hB[i], hA_2[i]));
            }
        }
    } // end of norm_check

    if(arg.timing)
    {
        ArgumentModel<e_uplo, e_diag, e_N, e_lda, e_batch_count>{}.log_args<T>(
            rocblas_cout,
            arg,
            gpu_time_used,
            trtri_gflop_count<T>(N),
            ArgumentLogging::NA_value,
            cpu_time_used,
            rocblas_error);
    }
}
