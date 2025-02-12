/* ************************************************************************
 * Copyright 2020-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "rocblas_device_malloc.hpp"
#include "utility.hpp"

int main()
{
    rocblas_handle handle;
    CHECK_ROCBLAS_ERROR(rocblas_create_handle(&handle));

    // Free all memory in the handle
    CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, 0));

    size_t sizes[] = {512, 1025, 10 * 1024 * 1024};

    if(rocblas_is_device_memory_size_query(handle))
    {
        rocblas_cerr << "rocblas_is_device_memory_size_query() incorrectly returned true"
                     << std::endl;
        return 1;
    }

    CHECK_ROCBLAS_ERROR(rocblas_start_device_memory_size_query(handle));

    if(!rocblas_is_device_memory_size_query(handle))
    {
        rocblas_cerr << "rocblas_is_device_memory_size_query() incorrectly returned false"
                     << std::endl;
        return 1;
    }

    CHECK_ALLOC_QUERY(rocblas_set_optimal_device_memory_size(handle, sizes[0], sizes[1], sizes[2]));

    size_t max;
    CHECK_ROCBLAS_ERROR(rocblas_stop_device_memory_size_query(handle, &max));

    CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, max));

    for(size_t size : sizes)
    {
        try
        {
            rocblas_device_malloc mem(handle, size);

            if(!mem)
            {
                rocblas_cerr << "rocblas_device_malloc failed" << std::endl;
                return 1;
            }

            void* ptr = static_cast<void*>(mem);

            hipMemset(ptr, 0, size);

            rocblas_device_malloc_base& ref = mem;
        }
        catch(...)
        {
            rocblas_cerr << "exception thrown for malloc size " << size << std::endl;
            return 1;
        }

        rocblas_device_malloc       mem2 = rocblas_device_malloc(handle, size);
        rocblas_device_malloc_base& ref2 = mem2;
        rocblas_device_malloc       mem3 = std::move(mem2);
    }

    try
    {
        rocblas_device_malloc mem(handle, sizes[0], sizes[1], sizes[2]);

        if(!mem)
        {
            rocblas_cerr << "rocblas_device_malloc failed" << std::endl;
            return 1;
        }

        for(size_t i = 0; i < 3; ++i)
        {
            void* ptr = mem[i];
            if(!ptr)
            {
                rocblas_cerr << "nullptr returned from rocblas_device_malloc[" << i << "]"
                             << std::endl;
                return 1;
            }
            hipMemset(ptr, 0, sizes[i]);
        }

        rocblas_device_malloc_base& ref = mem;
    }
    catch(...)
    {
        rocblas_cerr << "exception thrown for malloc size " << sizes[0] << "," << sizes[1] << ","
                     << sizes[2] << std::endl;
        return 1;
    }

    CHECK_ROCBLAS_ERROR(rocblas_destroy_handle(handle));

    rocblas_cout << "All tests passed" << std::endl;
}
