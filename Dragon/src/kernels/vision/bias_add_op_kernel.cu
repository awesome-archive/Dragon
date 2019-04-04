#ifdef WITH_CUDA

#include "core/context_cuda.h"
#include "utils/op_kernel.h"

namespace dragon {

namespace kernel {

/*! BiasAdd <T = float32, Device = CUDA> */

template <typename T>
__global__ void _BiasAdd_NCHW(
    const int               nthreads,
    const int               dim,
    const int               inner_dim,
    const T*                bias,
    T*                      y) {
    CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 350
        y[i] += __ldg(bias + ((i / inner_dim) % dim));
#else
        y[i] += bias[(i / inner_dim) % dim];
#endif
    }
}

template <typename T>
__global__ void _BiasAdd_NHWC(
    const int               nthreads,
    const int               dim,
    const int               inner_dim,
    const T*                bias,
    T*                      y) {
    CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 350
        y[i] += __ldg(bias + (i % dim));
#else
        y[i] += bias[i % dim];
#endif
    }
}

template<> void BiasAdd<float, CUDAContext>(
    const int               outer_dim,
    const int               dim,
    const int               inner_dim,
    const string&           data_format,
    const float*            bias,
    const float*            bias_multiplier,
    float*                  y,
    CUDAContext*            ctx) {
    auto nthreads = outer_dim * dim * inner_dim;
    if (data_format == "NCHW") {
        _BiasAdd_NCHW<float>
            << < CUDA_BLOCKS(nthreads), CUDA_THREADS,
                 0, ctx->cuda_stream() >> >
            (nthreads, dim, inner_dim, bias, y);
    } else if (data_format == "NHWC") {
        _BiasAdd_NHWC<float>
            << < CUDA_BLOCKS(nthreads), CUDA_THREADS,
                 0, ctx->cuda_stream() >> >
            (nthreads, dim, inner_dim, bias, y);
    } else LOG(FATAL) << "Unknown data format: " << data_format;
}

}  // namespace kernel

}  // namepsace dragon

#endif  // WITH_CUDA