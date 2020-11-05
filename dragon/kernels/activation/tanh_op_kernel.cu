#ifdef USE_CUDA

#include "dragon/core/context_cuda.h"
#include "dragon/utils/math_functions.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

namespace kernel {

namespace {

template <typename T>
__global__ void _Tanh(const int nthreads, const T* x, T* y) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    y[i] = tanh(x[i]);
  }
}

template <>
__global__ void _Tanh<half>(const int nthreads, const half* x, half* y) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    y[i] = __float2half(tanh(__half2float(x[i])));
  }
}

template <>
__global__ void _Tanh<half2>(const int nthreads, const half2* x, half2* y) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    const float2 val = __half22float2(x[i]);
    y[i] = __floats2half2_rn(tanh(val.x), tanh(val.y));
  }
}

template <typename T>
__global__ void _TanhGrad(const int nthreads, const T* dy, const T* y, T* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    dx[i] = dy[i] * (T(1) - utils::math::Square(y[i]));
  }
}

template <>
__global__ void
_TanhGrad<half>(const int nthreads, const half* dy, const half* y, half* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    dx[i] = __float2half(
        __half2float(dy[i]) * (1.f - utils::math::Square(__half2float(y[i]))));
  }
}

template <>
__global__ void _TanhGrad<half2>(
    const int nthreads,
    const half2* dy,
    const half2* y,
    half2* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    const float2 val = __half22float2(y[i]);
    const float2 grad = __half22float2(dy[i]);
    dx[i] = __floats2half2_rn(
        grad.x * (1.f - utils::math::Square(val.x)),
        grad.y * (1.f - utils::math::Square(val.y)));
  }
}

} // namespace

/* ------------------- Launcher Separator ------------------- */

template <>
void Tanh<float16, CUDAContext>(
    const int count,
    const float16* x,
    float16* y,
    CUDAContext* ctx) {
  if ((count & 1) == 0) {
    _Tanh<<<CUDA_BLOCKS(count >> 1), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
        count >> 1,
        reinterpret_cast<const half2*>(x),
        reinterpret_cast<half2*>(y));
  } else {
    _Tanh<<<CUDA_BLOCKS(count), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
        count, reinterpret_cast<const half*>(x), reinterpret_cast<half*>(y));
  }
}

template <>
void TanhGrad<float16, CUDAContext>(
    const int count,
    const float16* dy,
    const float16* y,
    float16* dx,
    CUDAContext* ctx) {
  if ((count & 1) == 0) {
    _TanhGrad<<<CUDA_BLOCKS(count >> 1), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
        count >> 1,
        reinterpret_cast<const half2*>(dy),
        reinterpret_cast<const half2*>(y),
        reinterpret_cast<half2*>(dx));
  } else {
    _TanhGrad<<<CUDA_BLOCKS(count), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
        count,
        reinterpret_cast<const half*>(dy),
        reinterpret_cast<const half*>(y),
        reinterpret_cast<half*>(dx));
  }
} // TanhGrad

#define DEFINE_KERNEL_LAUNCHER(T)                                       \
  template <>                                                           \
  void Tanh<T, CUDAContext>(                                            \
      const int count, const T* x, T* y, CUDAContext* ctx) {            \
    _Tanh<<<CUDA_BLOCKS(count), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
        count, x, y);                                                   \
  }

#define DEFINE_GRAD_KERNEL_LAUNCHER(T)                                      \
  template <>                                                               \
  void TanhGrad<T, CUDAContext>(                                            \
      const int count, const T* dy, const T* y, T* dx, CUDAContext* ctx) {  \
    _TanhGrad<<<CUDA_BLOCKS(count), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
        count, dy, y, dx);                                                  \
  }

DEFINE_KERNEL_LAUNCHER(float);
DEFINE_KERNEL_LAUNCHER(double);

DEFINE_GRAD_KERNEL_LAUNCHER(float);
DEFINE_GRAD_KERNEL_LAUNCHER(double);

#undef DEFINE_KERNEL_LAUNCHER
#undef DEFINE_GRAD_KERNEL_LAUNCHER

} // namespace kernel

} // namespace dragon

#endif // USE_CUDA
