#ifdef USE_CUDA

#include "dragon/core/context_cuda.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

namespace kernels {

namespace {

template <typename IndexT, typename ValueT>
__global__ void
_BooleanMask(const int N, const IndexT* index, const ValueT* x, ValueT* y) {
  CUDA_1D_KERNEL_LOOP(i, N) {
    y[i] = x[index[i]];
  }
}

template <typename IndexT, typename ValueT>
__global__ void _BooleanMaskGrad(
    const int N,
    const IndexT* index,
    const ValueT* dy,
    ValueT* dx) {
  CUDA_1D_KERNEL_LOOP(i, N) {
    dx[index[i]] = dy[i];
  }
}

} // namespace

/* ------------------- Launcher Separator ------------------- */

#define DEFINE_KERNEL_LAUNCHER(IndexT, ValueT)                             \
  template <>                                                              \
  void BooleanMask<IndexT, ValueT, CUDAContext>(                           \
      const int N,                                                         \
      const IndexT* index,                                                 \
      const ValueT* x,                                                     \
      ValueT* y,                                                           \
      CUDAContext* ctx) {                                                  \
    _BooleanMask<<<CUDA_BLOCKS(N), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
        N, index, x, y);                                                   \
  }

#define DEFINE_GRAD_KERNEL_LAUNCHER(IndexT, ValueT)                            \
  template <>                                                                  \
  void BooleanMaskGrad<IndexT, ValueT, CUDAContext>(                           \
      const int N,                                                             \
      const IndexT* index,                                                     \
      const ValueT* dy,                                                        \
      ValueT* dx,                                                              \
      CUDAContext* ctx) {                                                      \
    _BooleanMaskGrad<<<CUDA_BLOCKS(N), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
        N, index, dy, dx);                                                     \
  }

DEFINE_KERNEL_LAUNCHER(int, bool);
DEFINE_KERNEL_LAUNCHER(int, uint8_t);
DEFINE_KERNEL_LAUNCHER(int, int8_t);
DEFINE_KERNEL_LAUNCHER(int, int);
DEFINE_KERNEL_LAUNCHER(int, int64_t);
DEFINE_KERNEL_LAUNCHER(int, float16);
DEFINE_KERNEL_LAUNCHER(int, float);
DEFINE_KERNEL_LAUNCHER(int, double);
DEFINE_KERNEL_LAUNCHER(int64_t, bool);
DEFINE_KERNEL_LAUNCHER(int64_t, uint8_t);
DEFINE_KERNEL_LAUNCHER(int64_t, int8_t);
DEFINE_KERNEL_LAUNCHER(int64_t, int);
DEFINE_KERNEL_LAUNCHER(int64_t, int64_t);
DEFINE_KERNEL_LAUNCHER(int64_t, float16);
DEFINE_KERNEL_LAUNCHER(int64_t, float);
DEFINE_KERNEL_LAUNCHER(int64_t, double);
DEFINE_GRAD_KERNEL_LAUNCHER(int, float16);
DEFINE_GRAD_KERNEL_LAUNCHER(int, float);
DEFINE_GRAD_KERNEL_LAUNCHER(int, double);
DEFINE_GRAD_KERNEL_LAUNCHER(int64_t, float16);
DEFINE_GRAD_KERNEL_LAUNCHER(int64_t, float);
DEFINE_GRAD_KERNEL_LAUNCHER(int64_t, double);
#undef DEFINE_KERNEL_LAUNCHER
#undef DEFINE_GRAD_KERNEL_LAUNCHER

} // namespace kernels

} // namespace dragon

#endif // USE_CUDA
