#ifdef USE_CUDA

#include "dragon/core/context_cuda.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

namespace kernel {

namespace {

template <typename T>
__global__ void _RMSPropUpdate(
    const int nthreads,
    const T lr,
    const T momentum,
    const T decay,
    const T eps,
    T* g,
    T* m,
    T* v) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    T gi = g[i];
    T vi = v[i] = decay * v[i] + (1 - decay) * gi * gi;
    g[i] = m[i] = momentum * m[i] + (lr * gi / (sqrt(vi) + eps));
  }
}

} // namespace

/* ------------------- Launcher Separator ------------------- */

template <>
void RMSPropUpdate<float, CUDAContext>(
    const int count,
    const float lr,
    const float momentum,
    const float decay,
    const float eps,
    float* g,
    float* m,
    float* v,
    CUDAContext* ctx) {
  _RMSPropUpdate<<<CUDA_BLOCKS(count), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
      count, lr, momentum, decay, eps, g, m, v);
}

} // namespace kernel

} // namespace dragon

#endif // USE_CUDA
