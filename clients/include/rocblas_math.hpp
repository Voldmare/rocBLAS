/* ************************************************************************
 * Copyright 2018-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once

#include "rocblas.h"
#include <cmath>
#include <hip/hip_runtime.h>
#include <immintrin.h>
#include <type_traits>

/* ============================================================================================ */
// Helper function to truncate float to bfloat16

inline __host__ rocblas_bfloat16 float_to_bfloat16_truncate(float val)
{
    union
    {
        float    fp32;
        uint32_t int32;
    } u = {val};
    rocblas_bfloat16 ret;
    ret.data = uint16_t(u.int32 >> 16);
    if((u.int32 & 0x7fff0000) == 0x7f800000 && u.int32 & 0xffff)
        ret.data |= 1; // Preserve signaling NaN
    return ret;
}

/* ============================================================================================ */
/*! \brief negate a value */

template <class T>
inline T negate(T x)
{
    return -x;
}

template <>
inline rocblas_half negate(rocblas_half arg)
{
    union
    {
        rocblas_half fp;
        uint16_t     data;
    } x = {arg};

    x.data ^= 0x8000;
    return x.fp;
}

template <>
inline rocblas_bfloat16 negate(rocblas_bfloat16 x)
{
    x.data ^= 0x8000;
    return x;
}

/* ============================================================================================ */
// Conjugate a value. For most types, simply return argument; for
// rocblas_float_complex and rocblas_double_complex, return std::conj(z)
template <typename T, std::enable_if_t<!is_complex<T>, int> = 0>
__host__ inline T conjugate(const T& z)
{
    return z;
}

template <typename T, std::enable_if_t<is_complex<T>, int> = 0>
__host__ inline T conjugate(const T& z)
{
    return std::conj(z);
}
