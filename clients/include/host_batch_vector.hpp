/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include <string.h>

#include "host_alloc.hpp"
#include "rocblas_init.hpp"

//
// Local declaration of the device batch vector.
//
template <typename T>
class device_batch_vector;

//!
//! @brief Implementation of the batch vector on host.
//!
template <typename T>
class host_batch_vector
{
public:
    //!
    //! @brief Delete copy constructor.
    //!
    host_batch_vector(const host_batch_vector<T>& that) = delete;

    //!
    //! @brief Delete copy assignement.
    //!
    host_batch_vector& operator=(const host_batch_vector<T>& that) = delete;

    //!
    //! @brief Constructor.
    //! @param n           The length of the vector.
    //! @param inc         The increment.
    //! @param batch_count The batch count.
    //!
    explicit host_batch_vector(size_t n, rocblas_int inc, rocblas_int batch_count)
        : m_n(n)
        , m_inc(inc)
        , m_batch_count(batch_count)
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
    //!
    explicit host_batch_vector(size_t         n,
                               rocblas_int    inc,
                               rocblas_stride stride,
                               rocblas_int    batch_count)
        : host_batch_vector(n, inc, batch_count)
    {
    }

    //!
    //! @brief Destructor.
    //!
    ~host_batch_vector()
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
    //! @brief Returns the batch count.
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
    //! @brief Random access to the vectors.
    //! @param batch_index the batch index.
    //! @return The mutable pointer.
    //!
    T* operator[](rocblas_int batch_index)
    {

        return this->m_data[batch_index];
    }

    //!
    //! @brief Constant random access to the vectors.
    //! @param batch_index the batch index.
    //! @return The non-mutable pointer.
    //!
    const T* operator[](rocblas_int batch_index) const
    {

        return this->m_data[batch_index];
    }

    //!
    //! @brief Cast to a double pointer.
    //!
    // clang-format off
    operator T**()
    // clang-format on
    {
        return this->m_data;
    }

    //!
    //! @brief Constant cast to a double pointer.
    //!
    operator const T* const *()
    {
        return this->m_data;
    }

    //!
    //! @brief Copy from a host batched vector.
    //! @param that the vector the data is copied from.
    //! @return true if the copy is done successfully, false otherwise.
    //!
    bool copy_from(const host_batch_vector<T>& that)
    {
        if((this->batch_count() == that.batch_count()) && (this->n() == that.n())
           && (this->inc() == that.inc()))
        {
            size_t num_bytes = this->n() * std::abs(this->inc()) * sizeof(T);
            for(rocblas_int batch_index = 0; batch_index < this->m_batch_count; ++batch_index)
            {
                memcpy((*this)[batch_index], that[batch_index], num_bytes);
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    //!
    //! @brief Transfer from a device batched vector.
    //! @param that the vector the data is copied from.
    //! @return the hip error.
    //!
    hipError_t transfer_from(const device_batch_vector<T>& that)
    {
        hipError_t hip_err;
        size_t     num_bytes = size_t(this->m_n) * std::abs(this->m_inc) * sizeof(T);
        if(that.use_HMM && hipSuccess != (hip_err = hipDeviceSynchronize()))
            return hip_err;

        hipMemcpyKind kind = that.use_HMM ? hipMemcpyHostToHost : hipMemcpyDeviceToHost;

        for(rocblas_int batch_index = 0; batch_index < this->m_batch_count; ++batch_index)
        {
            if(hipSuccess
               != (hip_err = hipMemcpy((*this)[batch_index], that[batch_index], num_bytes, kind)))
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
        return (nullptr != this->m_data) ? hipSuccess : hipErrorOutOfMemory;
    }

private:
    size_t      m_n{}; // This may hold a matrix so using size_t.
    rocblas_int m_inc{};
    rocblas_int m_batch_count{};
    T**         m_data{};

    bool try_initialize_memory()
    {
        bool success
            = (nullptr != (this->m_data = (T**)host_calloc_throw(this->m_batch_count, sizeof(T*))));
        if(success)
        {
            size_t nmemb = size_t(this->m_n) * std::abs(this->m_inc);
            for(rocblas_int batch_index = 0; batch_index < this->m_batch_count; ++batch_index)
            {
                success
                    = (nullptr
                       != (this->m_data[batch_index] = (T*)host_malloc_throw(nmemb, sizeof(T))));
                if(false == success)
                {
                    break;
                }
            }
        }
        return success;
    }

    void free_memory()
    {
        if(nullptr != this->m_data)
        {
            for(rocblas_int batch_index = 0; batch_index < this->m_batch_count; ++batch_index)
            {
                if(nullptr != this->m_data[batch_index])
                {
                    free(this->m_data[batch_index]);
                    this->m_data[batch_index] = nullptr;
                }
            }

            free(this->m_data);
            this->m_data = nullptr;
        }
    }
};

//!
//! @brief Overload output operator.
//! @param os The ostream.
//! @param that That host batch vector.
//!
template <typename T>
rocblas_internal_ostream& operator<<(rocblas_internal_ostream& os, const host_batch_vector<T>& that)
{
    auto n           = that.n();
    auto inc         = std::abs(that.inc());
    auto batch_count = that.batch_count();

    for(rocblas_int batch_index = 0; batch_index < batch_count; ++batch_index)
    {
        auto batch_data = that[batch_index];
        os << "[" << batch_index << "] = { " << batch_data[0];
        for(size_t i = 1; i < n; ++i)
        {
            os << ", " << batch_data[i * inc];
        }
        os << " }" << std::endl;
    }

    return os;
}
