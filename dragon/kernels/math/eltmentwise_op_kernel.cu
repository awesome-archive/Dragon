#ifdef USE_CUDA

#include "dragon/core/context_cuda.h"
#include "dragon/utils/math_functions.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

namespace kernel {

namespace {

template <typename T>
__global__ void _CosGrad(const int nthreads, const T* dy, const T* x, T* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    dx[i] = -dy[i] * sin(x[i]);
  }
}

template <>
__global__ void
_CosGrad<half>(const int nthreads, const half* dy, const half* x, half* dx) {
  const half kFactor = __float2half(-1.f);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul(__hmul(dy[i], kFactor), hsin(x[i]));
#endif
  }
}

template <>
__global__ void _CosGrad<half2>(
    const int nthreads,
    const half2* dy,
    const half2* x,
    half2* dx) {
  const half2 kFactor = __float2half2_rn(-1.f);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul2(__hmul2(dy[i], kFactor), h2sin(x[i]));
#endif
  }
}

template <typename T>
__global__ void _SinGrad(const int nthreads, const T* dy, const T* x, T* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    dx[i] = dy[i] * cos(x[i]);
  }
}

template <>
__global__ void
_SinGrad<half>(const int nthreads, const half* dy, const half* x, half* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul(dy[i], hcos(x[i]));
#endif
  }
}

template <>
__global__ void _SinGrad<half2>(
    const int nthreads,
    const half2* dy,
    const half2* x,
    half2* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul2(dy[i], h2cos(x[i]));
#endif
  }
}

template <typename T>
__global__ void
_ReciprocalGrad(const int nthreads, const T* dy, const T* y, T* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    dx[i] = -dy[i] * utils::math::Square(y[i]);
  }
}

template <>
__global__ void _ReciprocalGrad<half>(
    const int nthreads,
    const half* dy,
    const half* y,
    half* dx) {
  const half c = __float2half(-1.f);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul(__hmul(c, dy[i]), utils::math::Square(y[i]));
#endif
  }
}

template <>
__global__ void _ReciprocalGrad<half2>(
    const int nthreads,
    const half2* dy,
    const half2* y,
    half2* dx) {
  const half2 c = __float2half2_rn(-1.f);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul2(__hmul2(c, dy[i]), utils::math::Square(y[i]));
#endif
  }
}

template <typename T>
__global__ void _RsqrtGrad(const int nthreads, const T* dy, const T* y, T* dx) {
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    dx[i] = T(-0.5) * dy[i] * utils::math::Cube(y[i]);
  }
}

template <>
__global__ void
_RsqrtGrad<half>(const int nthreads, const half* dy, const half* y, half* dx) {
  const half c = __float2half(-0.5f);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul(__hmul(c, dy[i]), utils::math::Cube(y[i]));
#endif
  }
}

template <>
__global__ void _RsqrtGrad<half2>(
    const int nthreads,
    const half2* dy,
    const half2* y,
    half2* dx) {
  const half2 c = __float2half2_rn(-0.5f);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
#if __CUDA_ARCH__ >= 530
    dx[i] = __hmul2(__hmul2(c, dy[i]), utils::math::Cube(y[i]));
#endif
  }
}

} // namespace

/* ------------------- Launcher Separator ------------------- */

#define DEFINE_GRAD_KERNEL_LAUNCHER(name, T)                               \
  template <>                                                              \
  void name##Grad<T, CUDAContext>(                                         \
      const int count, const T* dy, const T* x, T* dx, CUDAContext* ctx) { \
    _##name##Grad<<<                                                       \
        CUDA_BLOCKS(count),                                                \
        CUDA_THREADS,                                                      \
        0,                                                                 \
        ctx->cuda_stream()>>>(count, dy, x, dx);                           \
  }

DEFINE_GRAD_KERNEL_LAUNCHER(Cos, float);
DEFINE_GRAD_KERNEL_LAUNCHER(Cos, double);
DEFINE_GRAD_KERNEL_LAUNCHER(Sin, float);
DEFINE_GRAD_KERNEL_LAUNCHER(Sin, double);
DEFINE_GRAD_KERNEL_LAUNCHER(Reciprocal, float);
DEFINE_GRAD_KERNEL_LAUNCHER(Reciprocal, double);
DEFINE_GRAD_KERNEL_LAUNCHER(Rsqrt, float);
DEFINE_GRAD_KERNEL_LAUNCHER(Rsqrt, double);
#undef DEFINE_GRAD_KERNEL_LAUNCHER

#define DEFINE_GRAD_KERNEL_LAUNCHER(name)     \
  template <>                                 \
  void name##Grad<float16, CUDAContext>(      \
      const int count,                        \
      const float16* dy,                      \
      const float16* x,                       \
      float16* dx,                            \
      CUDAContext* ctx) {                     \
    if ((count & 1) == 0) {                   \
      _##name##Grad<<<                        \
          CUDA_BLOCKS(count >> 1),            \
          CUDA_THREADS,                       \
          0,                                  \
          ctx->cuda_stream()>>>(              \
          count >> 1,                         \
          reinterpret_cast<const half2*>(dy), \
          reinterpret_cast<const half2*>(x),  \
          reinterpret_cast<half2*>(dx));      \
    } else {                                  \
      _##name##Grad<<<                        \
          CUDA_BLOCKS(count),                 \
          CUDA_THREADS,                       \
          0,                                  \
          ctx->cuda_stream()>>>(              \
          count,                              \
          reinterpret_cast<const half*>(dy),  \
          reinterpret_cast<const half*>(x),   \
          reinterpret_cast<half*>(dx));       \
    }                                         \
  }

DEFINE_GRAD_KERNEL_LAUNCHER(Cos);
DEFINE_GRAD_KERNEL_LAUNCHER(Sin);
DEFINE_GRAD_KERNEL_LAUNCHER(Reciprocal);
DEFINE_GRAD_KERNEL_LAUNCHER(Rsqrt);
#undef DEFINE_GRAD_KERNEL_LAUNCHER

} // namespace kernel

} // namespace dragon

#endif // USE_CUDA
