#include "check_numerics_matrix.hpp"
#include "utility.hpp"

/**
  *
  * rocblas_internal_check_numerics_ge_matrix_template(function_name, handle, n, x, offset_x, inc_x, stride_x, batch_count, check_numerics, is_input)
  *
  * Info about rocblas_internal_check_numerics_ge_matrix_template function:
  *
  *    It is the host function which accepts a matrix and calls the 'rocblas_check_numerics_ge_matrix_kernel' kernel function
  *    to check for numerical abnormalities such as NaN/zero/Inf/denormal in that matrix.
  *    It also helps in debugging based on the different types of flags in rocblas_check_numerics_mode that users set to debug potential NaN/zero/Inf/denormal value.
  *    ge in rocblas_internal_check_numerics_ge_matrix_template refers to general.
  *
  * Parameters   : function_name         : Name of the rocBLAS math function
  *                handle                : Handle to the rocblas library context queue
  *                m                     : number of rows of matrix 'A'
  *                n                     : number of columns of matrix 'A'
  *                A                     : Pointer to the matrix which is under check for numerical abnormalities
  *                offset_a              : Offset of matrix 'A'
  *                lda                   : specifies the leading dimension of matrix 'A'
  *                stride_a              : Specifies the pointer increment between one matrix 'A_i' and the next one (A_i+1) (where (A_i) is the i-th instance of the batch)
  *                check_numerics        : User defined flag for debugging
  *                is_input              : To check if the matrix under consideration is an Input or an Output matrix
  *
  * Return Value : rocblas_status
  *        rocblas_status_success        : Return status if the matrix does not have a NaN/Inf/denormal value
  *   rocblas_status_check_numerics_fail : Return status if the matrix contains a NaN/Inf/denormal value and 'check_numerics' enum is set to 'rocblas_check_numerics_mode_fail'
  *
**/

template <typename T>
ROCBLAS_INTERNAL_EXPORT_NOINLINE rocblas_status
    rocblas_internal_check_numerics_ge_matrix_template(const char*       function_name,
                                                       rocblas_handle    handle,
                                                       rocblas_operation trans_a,
                                                       rocblas_int       m,
                                                       rocblas_int       n,
                                                       T                 A,
                                                       rocblas_stride    offset_a,
                                                       rocblas_int       lda,
                                                       rocblas_stride    stride_a,
                                                       rocblas_int       batch_count,
                                                       const int         check_numerics,
                                                       bool              is_input)
{
    //Quick return if possible. Not Argument error
    if(!m || !n || !batch_count || !A)
        return rocblas_status_success;

    //Creating structure host object
    rocblas_check_numerics_t h_abnormal;

    //Allocating memory for device structure
    auto d_abnormal = handle->device_malloc(sizeof(rocblas_check_numerics_t));

    //Transferring the rocblas_check_numerics_t structure from host to the device
    RETURN_IF_HIP_ERROR(hipMemcpy((rocblas_check_numerics_t*)d_abnormal,
                                  &h_abnormal,
                                  sizeof(rocblas_check_numerics_t),
                                  hipMemcpyHostToDevice));

    //Checking trans_a to transpose a matrix 'A'
    rocblas_int num_rows_a = trans_a == rocblas_operation_none ? m : n;
    rocblas_int num_cols_a = trans_a == rocblas_operation_none ? n : m;

    hipStream_t          rocblas_stream = handle->get_stream();
    static constexpr int DIM_X          = 16;
    static constexpr int DIM_Y          = 16;
    rocblas_int          blocks_X       = (num_rows_a - 1) / DIM_X + 1;
    rocblas_int          blocks_Y       = (num_cols_a - 1) / DIM_Y + 1;

    dim3 blocks(blocks_X, blocks_Y, batch_count);
    dim3 threads(DIM_X, DIM_Y);

    hipLaunchKernelGGL(rocblas_check_numerics_ge_matrix_kernel,
                       blocks,
                       threads,
                       0,
                       rocblas_stream,
                       num_rows_a,
                       num_cols_a,
                       A,
                       offset_a,
                       lda,
                       stride_a,
                       (rocblas_check_numerics_t*)d_abnormal);

    //Transferring the rocblas_check_numerics_t structure from device to the host
    RETURN_IF_HIP_ERROR(hipMemcpy(&h_abnormal,
                                  (rocblas_check_numerics_t*)d_abnormal,
                                  sizeof(rocblas_check_numerics_t),
                                  hipMemcpyDeviceToHost));

    return rocblas_check_numerics_abnormal_struct(
        function_name, check_numerics, is_input, &h_abnormal);
}

//ADDED INSTANTIATION TO SUPPORT T* AND T* CONST*

#ifdef INST
#error INST IS ALREADY DEFINED
#endif
#define INST(typet_)                                                                         \
    template ROCBLAS_INTERNAL_EXPORT_NOINLINE rocblas_status                                 \
        rocblas_internal_check_numerics_ge_matrix_template(const char*       function_name,  \
                                                           rocblas_handle    handle,         \
                                                           rocblas_operation trans_A,        \
                                                           rocblas_int       m,              \
                                                           rocblas_int       n,              \
                                                           typet_            A,              \
                                                           rocblas_stride    offset_a,       \
                                                           rocblas_int       lda,            \
                                                           rocblas_stride    stride_a,       \
                                                           rocblas_int       batch_count,    \
                                                           const int         check_numerics, \
                                                           bool              is_input)
INST(float*);
INST(double*);
INST(float* const*);
INST(double* const*);
INST(float const*);
INST(double const*);
INST(const float* const*);
INST(const double* const*);
INST(rocblas_float_complex*);
INST(rocblas_double_complex*);
INST(rocblas_float_complex* const*);
INST(rocblas_double_complex* const*);
INST(const rocblas_float_complex* const*);
INST(const rocblas_double_complex* const*);
INST(rocblas_float_complex const*);
INST(rocblas_double_complex const*);
INST(rocblas_half*);
INST(rocblas_bfloat16*);
INST(rocblas_half* const*);
INST(rocblas_bfloat16* const*);
INST(const rocblas_half* const*);
INST(const rocblas_bfloat16* const*);
INST(rocblas_half const*);
INST(rocblas_bfloat16 const*);
#undef INST
