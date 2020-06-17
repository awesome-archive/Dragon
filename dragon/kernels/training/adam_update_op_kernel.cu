#ifdef USE_CUDA

#include "dragon/core/context_cuda.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

namespace kernel {

namespace {

template <typename T>
__global__ void _AdamUpdate(
    const int nthreads,
    const T lr,
    const T beta1,
    const T beta2,
    const T eps,
    T* g,
    T* m,
    T* v) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    T gi = g[i];
    T mi = m[i] = m[i] * beta1 + gi * (1 - beta1);
    T vi = v[i] = v[i] * beta2 + gi * gi * (1 - beta2);
    g[i] = lr * mi / (sqrt(vi) + eps);
  }
}

} // namespace

/* ------------------- Launcher Separator ------------------- */

template <>
void AdamUpdate<float, CUDAContext>(
    const int count,
    const float lr,
    const float beta1,
    const float beta2,
    const float eps,
    float* g,
    float* m,
    float* v,
    CUDAContext* ctx) {
  _AdamUpdate<<<CUDA_BLOCKS(count), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
      count, lr, beta1, beta2, eps, g, m, v);
}

} // namespace kernel

} // namespace dragon

#endif // USE_CUDA
