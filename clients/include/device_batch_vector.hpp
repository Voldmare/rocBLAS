/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "d_vector.hpp"

//
// Local declaration of the host strided batch vector.
//
template <typename T>
class host_batch_vector;

//!
//! @brief  pseudo-vector subclass which uses a batch of device memory pointers and
//!  - an array of pointers in host memory
//!  - an array of pointers in device memory
//!
template <typename T>
class device_batch_vector : public d_vector<T>
{
public:
    //!
    //! @brief Disallow copying.
    //!
    device_batch_vector(const device_batch_vector&) = delete;

    //!
    //! @brief Disallow assigning.
    //!
    device_batch_vector& operator=(const device_batch_vector&) = delete;

    //!
    //! @brief Constructor.
    //! @param n           The length of the vector.
    //! @param inc         The increment.
    //! @param batch_count The batch count.
    //! @param HMM         HipManagedMemory Flag.
    //!
    explicit device_batch_vector(size_t      n,
                                 rocblas_int inc,
                                 rocblas_int batch_count,
                                 bool        HMM = false)
        : m_n(n)
        , m_inc(inc)
        , m_batch_count(batch_count)
        , d_vector<T>(size_t(n) * std::abs(inc), HMM)
    {
        if(false == this->try_initialize_memory())
        {
            this->free_memory();
        }
    }

    //!
    //! @brief Constructor.
    //! @param n           The length of the vector.
    //! @param inc         The increment.
    //! @param stride      (UNUSED) The stride.
    //! @param batch_count The batch count.
    //! @param HMM         HipManagedMemory Flag.
    //!
    explicit device_batch_vector(
        size_t n, rocblas_int inc, rocblas_stride stride, rocblas_int batch_count, bool HMM = false)
        : device_batch_vector(n, inc, batch_count, HMM)
    {
    }

    //!
    //! @brief Destructor.
    //!
    ~device_batch_vector()
    {
        this->free_memory();
    }

    //!
    //! @brief Returns the length of the vector.
    //!
    size_t n() const
    {
        return this->m_n;
    }

    //!
    //! @brief Returns the increment of the vector.
    //!
    rocblas_int inc() const
    {
        return this->m_inc;
    }

    //!
    //! @brief Returns the value of batch_count.
    //!
    rocblas_int batch_count() const
    {
        return this->m_batch_count;
    }

    //!
    //! @brief Returns the stride value.
    //!
    rocblas_stride stride() const
    {
        return 0;
    }

    //!
    //! @brief Access to device data.
    //! @return Pointer to the device data.
    //!
    T** ptr_on_device()
    {
        return this->m_device_data;
    }

    //!
    //! @brief Const access to device data.
    //! @return Const pointer to the device data.
    //!
    const T* const* ptr_on_device() const
    {
        return this->m_device_data;
    }

    //!
    //! @brief access to device data.
    //! @return Const pointer to the device data.
    //!
    T* const* const_batch_ptr()
    {
        return this->m_device_data;
    }

    //!
    //! @brief Random access.
    //! @param batch_index The batch index.
    //! @return Pointer to the array on device.
    //!
    T* operator[](rocblas_int batch_index)
    {

        return this->m_data[batch_index];
    }

    //!
    //! @brief Constant random access.
    //! @param batch_index The batch index.
    //! @return Constant pointer to the array on device.
    //!
    const T* operator[](rocblas_int batch_index) const
    {

        return this->m_data[batch_index];
    }

    //!
    //! @brief Const cast of the data on host.
    //!
    operator const T* const *() const
    {
        return this->m_data;
    }

    //!
    //! @brief Cast of the data on host.
    //!
    // clang-format off
    operator T**()
    // clang-format on
    {
        return this->m_data;
    }

    //!
    //! @brief Tell whether ressources allocation failed.
    //!
    explicit operator bool() const
    {
        return nullptr != this->m_data;
    }

    //!
    //! @brief Copy from a host batched vector.
    //! @param that The host_batch_vector to copy.
    //!
    hipError_t transfer_from(const host_batch_vector<T>& that)
    {
        hipError_t hip_err;
        //
        // Copy each vector.
        //
        hipMemcpyKind kind = this->use_HMM ? hipMemcpyHostToHost : hipMemcpyHostToDevice;
        for(rocblas_int batch_index = 0; batch_index < this->m_batch_count; ++batch_index)
        {
            if(hipSuccess
               != (hip_err = hipMemcpy(
                       (*this)[batch_index], that[batch_index], sizeof(T) * this->nmemb(), kind)))
            {
                return hip_err;
            }
        }

        return hipSuccess;
    }

    //!
    //! @brief Check if memory exists.
    //! @return hipSuccess if memory exists, hipErrorOutOfMemory otherwise.
    //!
    hipError_t memcheck() const
    {
        if(*this)
            return hipSuccess;
        else
            return hipErrorOutOfMemory;
    }

private:
    size_t      m_n{};
    rocblas_int m_inc{};
    rocblas_int m_batch_count{};
    T**         m_data{};
    T**         m_device_data{};

    //!
    //! @brief Try to allocate the ressources.
    //! @return true if success false otherwise.
    //!
    bool try_initialize_memory()
    {
        bool success = false;

        success
            = (hipSuccess
               == (!this->use_HMM
                       ? (hipMalloc)(&this->m_device_data, this->m_batch_count * sizeof(T*))
                       : hipMallocManaged(&this->m_device_data, this->m_batch_count * sizeof(T*))));
        if(success)
        {
            success
                = (nullptr
                   != (this->m_data = !this->use_HMM ? (T**)calloc(this->m_batch_count, sizeof(T*))
                                                     : m_device_data));
            if(success)
            {
                for(rocblas_int batch_index = 0; batch_index < this->m_batch_count; ++batch_index)
                {
                    success
                        = (nullptr != (this->m_data[batch_index] = this->device_vector_setup()));
                    if(!success)
                    {
                        break;
                    }
                }

                if(success && !this->use_HMM)
                {
                    success = (hipSuccess
                               == hipMemcpy(this->m_device_data,
                                            this->m_data,
                                            sizeof(T*) * this->m_batch_count,
                                            hipMemcpyHostToDevice));
                }
            }
        }
        return success;
    }

    //!
    //! @brief Free the ressources, as much as we can.
    //!
    void free_memory()
    {
        if(nullptr != this->m_data)
        {
            for(rocblas_int batch_index = 0; batch_index < this->m_batch_count; ++batch_index)
            {
                if(nullptr != this->m_data[batch_index])
                {
                    this->device_vector_teardown(this->m_data[batch_index]);
                    this->m_data[batch_index] = nullptr;
                }
            }

            if(!this->use_HMM)
            {
                free(this->m_data);
            }
            // else this is just a copy of m_device_data

            this->m_data = nullptr;
        }

        if(nullptr != this->m_device_data)
        {
            auto tmp_device_data = this->m_device_data;
            this->m_device_data  = nullptr;
            CHECK_HIP_ERROR((hipFree)(tmp_device_data));
        }
    }
};
