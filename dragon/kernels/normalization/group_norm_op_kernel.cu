#ifdef USE_CUDA

#include "dragon/core/memory.h"
#include "dragon/utils/device/common_cub.h"
#include "dragon/utils/math_functions.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

namespace kernel {

#if __CUDA_ARCH__ >= 350
#define LOAD(x, i) __ldg(x + i)
#define LOADF(x, i) __half2float(__ldg(x + i))
#else
#define LOAD(x, i) x[i]
#define LOADF(x, i) __half2float(x[i])
#endif

namespace {

template <typename T>
__global__ void _GroupNormFusedParams(
    const int N,
    const int G,
    const int D,
    const T* mu,
    const T* rsig,
    const T* gamma,
    const T* beta,
    T* scale,
    T* bias) {
  const int outer_dim = N * G;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    const int g = i % G;
    const T mu_val = LOAD(mu, i);
    const T rsig_val = LOAD(rsig, i);
    CUDA_2D_KERNEL_LOOP2(j, D) {
      const int wi = i * D + j;
      const int gi = g * D + j;
      const T w = LOAD(gamma, gi) * rsig_val;
      scale[wi] = w;
      bias[wi] = fma(-w, mu_val, LOAD(beta, gi));
    }
  }
}

template <typename Tx, typename Tp>
__global__ void _GroupNormForwardNCHW(
    const int N,
    const int C,
    const int S,
    const Tx* x,
    const Tp* scale,
    const Tp* bias,
    Tx* y) {
  const int outer_dim = N * C;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    const Tp w = LOAD(scale, i);
    const Tp b = LOAD(bias, i);
    CUDA_2D_KERNEL_LOOP2(j, S) {
      const int xi = i * S + j;
      y[xi] = fma(LOAD(x, xi), w, b);
    }
  }
}

template <>
__global__ void _GroupNormForwardNCHW<half, float>(
    const int N,
    const int C,
    const int S,
    const half* x,
    const float* scale,
    const float* bias,
    half* y) {
  const int outer_dim = N * C;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    const float w = LOAD(scale, i);
    const float b = LOAD(bias, i);
    CUDA_2D_KERNEL_LOOP2(j, S) {
      const int xi = i * S + j;
      y[xi] = __float2half(fmaf(LOADF(x, xi), w, b));
    }
  }
}

template <typename Tx, typename Tp>
__global__ void _GroupNormForwardNHWC(
    const int N,
    const int C,
    const int S,
    const Tx* x,
    const Tp* scale,
    const Tp* bias,
    Tx* y) {
  const int outer_dim = N * S;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    const int n = i / S;
    CUDA_2D_KERNEL_LOOP2(j, C) {
      const int xi = i * C + j;
      const int wi = n * C + j;
      y[xi] = fma(LOAD(x, xi), LOAD(scale, wi), LOAD(bias, wi));
    }
  }
}

template <>
__global__ void _GroupNormForwardNHWC<half, float>(
    const int N,
    const int C,
    const int S,
    const half* x,
    const float* scale,
    const float* bias,
    half* y) {
  const int outer_dim = N * S;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    const int n = i / S;
    CUDA_2D_KERNEL_LOOP2(j, C) {
      const int xi = i * C + j;
      const int wi = n * C + j;
      y[xi] = __float2half(fmaf(LOADF(x, xi), LOAD(scale, wi), LOAD(bias, wi)));
    }
  }
}

template <typename Tx, typename Tp, StorageOrder kOrder>
__global__ void _GroupNormWGrad(
    const int N,
    const int G,
    const int D,
    const int S,
    const Tx* x,
    const Tp* mu,
    const Tp* rsig,
    const Tx* dy,
    Tp* dgamma,
    Tp* dbeta) {
  const int outer_dim = G * D;
  const int inner_dim = N * S;
  __shared__ typename BlockReduce<Tp>::TempStorage dg_storage;
  __shared__ typename BlockReduce<Tp>::TempStorage db_storage;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    Tp dg_val = Tp(0), db_val = Tp(0);
    CUDA_2D_KERNEL_LOOP2(j, inner_dim) {
      const int n = j / S;
      const int xi = kOrder == StorageOrder::NCHW
          ? (n * outer_dim + i) * S + j % S
          : j * outer_dim + i;
      const int mi = n * G + i / D;
      dg_val += LOAD(dy, xi) * (LOAD(x, xi) - LOAD(mu, mi)) * LOAD(rsig, mi);
      db_val += LOAD(dy, xi);
    }
    dg_val = BlockReduce<Tp>(dg_storage).Reduce(dg_val, cub::Sum());
    db_val = BlockReduce<Tp>(db_storage).Reduce(db_val, cub::Sum());
    if (threadIdx.x == 0) {
      dgamma[i] = dg_val;
      dbeta[i] = db_val;
    }
  }
}

template <StorageOrder kOrder>
__global__ void _GroupNormWGradHalf(
    const int N,
    const int G,
    const int D,
    const int S,
    const half* x,
    const float* mu,
    const float* rsig,
    const half* dy,
    float* dgamma,
    float* dbeta) {
  const int outer_dim = G * D;
  const int inner_dim = N * S;
  __shared__ typename BlockReduce<float>::TempStorage dg_storage;
  __shared__ typename BlockReduce<float>::TempStorage db_storage;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    float dg_val = 0.f, db_val = 0.f;
    CUDA_2D_KERNEL_LOOP2(j, inner_dim) {
      const int n = j / S;
      const int xi = kOrder == StorageOrder::NCHW
          ? (n * outer_dim + i) * S + j % S
          : j * outer_dim + i;
      const int mi = n * G + i / D;
      dg_val += LOADF(dy, xi) * (LOADF(x, xi) - LOAD(mu, mi)) * LOAD(rsig, mi);
      db_val += LOADF(dy, xi);
    }
    dg_val = BlockReduce<float>(dg_storage).Reduce(dg_val, cub::Sum());
    db_val = BlockReduce<float>(db_storage).Reduce(db_val, cub::Sum());
    if (threadIdx.x == 0) {
      dgamma[i] = dg_val;
      dbeta[i] = db_val;
    }
  }
}

template <typename Tx, typename Tp, StorageOrder kOrder>
__global__ void _GroupNormInternalGrad(
    const int N,
    const int G,
    const int D,
    const int S,
    const Tx* x,
    const Tp* gamma,
    const Tx* dy,
    Tp* ds,
    Tp* db) {
  const int outer_dim = N * G;
  const int inner_dim = D * S;
  __shared__ typename BlockReduce<Tp>::TempStorage ds_storage;
  __shared__ typename BlockReduce<Tp>::TempStorage db_storage;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    Tp ds_val = Tp(0), db_val = Tp(0);
    CUDA_2D_KERNEL_LOOP2(j, inner_dim) {
      const int gi = i % G * D + j / S;
      const int xi = kOrder == StorageOrder::NCHW
          ? i * inner_dim + j
          : (i / G * S + j % S) * G * D + gi;
      ds_val += LOAD(gamma, gi) * LOAD(dy, xi) * LOAD(x, xi);
      db_val += LOAD(gamma, gi) * LOAD(dy, xi);
    }
    ds_val = BlockReduce<Tp>(ds_storage).Reduce(ds_val, cub::Sum());
    db_val = BlockReduce<Tp>(db_storage).Reduce(db_val, cub::Sum());
    if (threadIdx.x == 0) {
      ds[i] = ds_val;
      db[i] = db_val;
    }
  }
}

template <StorageOrder kOrder>
__global__ void _GroupNormInternalGradHalf(
    const int N,
    const int G,
    const int D,
    const int S,
    const half* x,
    const float* gamma,
    const half* dy,
    float* ds,
    float* db) {
  const int outer_dim = N * G;
  const int inner_dim = D * S;
  __shared__ typename BlockReduce<float>::TempStorage ds_storage;
  __shared__ typename BlockReduce<float>::TempStorage db_storage;
  CUDA_2D_KERNEL_LOOP1(i, outer_dim) {
    float ds_val = 0.f, db_val = 0.f;
    CUDA_2D_KERNEL_LOOP2(j, inner_dim) {
      const int gi = i % G * D + j / S;
      const int xi = kOrder == StorageOrder::NCHW
          ? i * inner_dim + j
          : (i / G * S + j % S) * G * D + gi;
      ds_val += LOAD(gamma, gi) * LOADF(dy, xi) * LOADF(x, xi);
      db_val += LOAD(gamma, gi) * LOADF(dy, xi);
    }
    ds_val = BlockReduce<float>(ds_storage).Reduce(ds_val, cub::Sum());
    db_val = BlockReduce<float>(db_storage).Reduce(db_val, cub::Sum());
    if (threadIdx.x == 0) {
      ds[i] = ds_val;
      db[i] = db_val;
    }
  }
}

template <typename Tx, typename Tp, StorageOrder kOrder>
__global__ void _GroupNormGrad(
    const int nthreads,
    const int G,
    const int D,
    const int S,
    const Tx* x,
    const Tp* mu,
    const Tp* rsig,
    const Tp* gamma,
    const Tp* ds,
    const Tp* db,
    const Tx* dy,
    Tx* dx) {
  const int C = G * D;
  const Tp denom = Tp(1) / Tp(D * S);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    const int mi = kOrder == StorageOrder::NCHW ? i / (D * S)
                                                : i / (C * S) * G + (i / D % G);
    const int gi = kOrder == StorageOrder::NCHW ? (i / S) % C : i % C;
    const Tp u = fma(LOAD(db, mi), LOAD(mu, mi), -LOAD(ds, mi)) *
        (LOAD(x, i) - LOAD(mu, mi)) * utils::math::Cube(LOAD(rsig, mi));
    const Tp v = LOAD(db, mi) * LOAD(rsig, mi);
    dx[i] = LOAD(gamma, gi) * LOAD(dy, i) * LOAD(rsig, mi) + (u - v) * denom;
  }
}

template <StorageOrder kOrder>
__global__ void _GroupNormGradHalf(
    const int nthreads,
    const int G,
    const int D,
    const int S,
    const half* x,
    const float* mu,
    const float* rsig,
    const float* gamma,
    const float* ds,
    const float* db,
    const half* dy,
    half* dx) {
  const int C = G * D;
  const float denom = 1.f / float(D * S);
  CUDA_1D_KERNEL_LOOP(i, nthreads) {
    const int mi = kOrder == StorageOrder::NCHW ? i / (D * S)
                                                : i / (C * S) * G + (i / D % G);
    const int gi = kOrder == StorageOrder::NCHW ? (i / S) % C : i % C;
    const float u = fmaf(LOAD(db, mi), LOAD(mu, mi), -LOAD(ds, mi)) *
        (LOADF(x, i) - LOAD(mu, mi)) * utils::math::Cube(LOAD(rsig, mi));
    const float v = LOAD(db, mi) * LOAD(rsig, mi);
    dx[i] = __float2half(
        LOAD(gamma, gi) * LOADF(dy, i) * LOAD(rsig, mi) + (u - v) * denom);
  }
}

#undef LOAD
#undef LOADF

} // namespace

/* ------------------- Launcher Separator ------------------- */

template <>
void GroupNormForward<float16, float, CUDAContext>(
    const int N,
    const int G,
    const int D,
    const int S,
    const string& data_format,
    const float16* x,
    const float* mu,
    const float* rsig,
    const float* gamma,
    const float* beta,
    float* scale,
    float* bias,
    float16* y,
    CUDAContext* ctx) {
  const int C = G * D;
  _GroupNormFusedParams<float>
      <<<CUDA_2D_BLOCKS(N * G), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
          N, G, D, mu, rsig, gamma, beta, scale, bias);
  if (data_format == "NCHW") {
    _GroupNormForwardNCHW<half, float>
        <<<CUDA_2D_BLOCKS(N * C), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            N,
            C,
            S,
            reinterpret_cast<const half*>(x),
            scale,
            bias,
            reinterpret_cast<half*>(y));
  } else if (data_format == "NHWC") {
    _GroupNormForwardNHWC<half, float>
        <<<CUDA_2D_BLOCKS(N * C), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            N,
            C,
            S,
            reinterpret_cast<const half*>(x),
            scale,
            bias,
            reinterpret_cast<half*>(y));
  }
}

template <>
void GroupNormBackward<float16, float, CUDAContext>(
    const int N,
    const int G,
    const int D,
    const int S,
    const string& data_format,
    const float16* x,
    const float* mu,
    const float* rsig,
    const float* gamma,
    const float16* dy,
    float* ds,
    float* db,
    float16* dx,
    float* dgamma,
    float* dbeta,
    CUDAContext* ctx) {
  auto nthreads = N * G * D * S;
  if (data_format == "NCHW") {
    _GroupNormWGradHalf<StorageOrder::NCHW>
        <<<CUDA_2D_BLOCKS(G * D), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            N,
            G,
            D,
            S,
            reinterpret_cast<const half*>(x),
            mu,
            rsig,
            reinterpret_cast<const half*>(dy),
            dgamma,
            dbeta);
    _GroupNormInternalGradHalf<StorageOrder::NCHW>
        <<<CUDA_2D_BLOCKS(N * G), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            N,
            G,
            D,
            S,
            reinterpret_cast<const half*>(x),
            gamma,
            reinterpret_cast<const half*>(dy),
            ds,
            db);
    _GroupNormGradHalf<StorageOrder::NCHW>
        <<<CUDA_BLOCKS(nthreads), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            nthreads,
            G,
            D,
            S,
            reinterpret_cast<const half*>(x),
            mu,
            rsig,
            gamma,
            ds,
            db,
            reinterpret_cast<const half*>(dy),
            reinterpret_cast<half*>(dx));
  } else if (data_format == "NHWC") {
    _GroupNormWGradHalf<StorageOrder::NHWC>
        <<<CUDA_2D_BLOCKS(G * D), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            N,
            G,
            D,
            S,
            reinterpret_cast<const half*>(x),
            mu,
            rsig,
            reinterpret_cast<const half*>(dy),
            dgamma,
            dbeta);
    _GroupNormInternalGradHalf<StorageOrder::NHWC>
        <<<CUDA_2D_BLOCKS(N * G), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            N,
            G,
            D,
            S,
            reinterpret_cast<const half*>(x),
            gamma,
            reinterpret_cast<const half*>(dy),
            ds,
            db);
    _GroupNormGradHalf<StorageOrder::NHWC>
        <<<CUDA_BLOCKS(nthreads), CUDA_THREADS, 0, ctx->cuda_stream()>>>(
            nthreads,
            G,
            D,
            S,
            reinterpret_cast<const half*>(x),
            mu,
            rsig,
            gamma,
            ds,
            db,
            reinterpret_cast<const half*>(dy),
            reinterpret_cast<half*>(dx));
  }
} // GroupNormBackward

#define DEFINE_KERNEL_LAUNCHER(Tx, Tp)                                     \
  template <>                                                              \
  void GroupNormForward<Tx, Tp, CUDAContext>(                              \
      const int N,                                                         \
      const int G,                                                         \
      const int D,                                                         \
      const int S,                                                         \
      const string& data_format,                                           \
      const Tx* x,                                                         \
      const Tp* mu,                                                        \
      const Tp* rsig,                                                      \
      const Tp* gamma,                                                     \
      const Tp* beta,                                                      \
      Tp* scale,                                                           \
      Tp* bias,                                                            \
      Tx* y,                                                               \
      CUDAContext* ctx) {                                                  \
    const int C = G * D;                                                   \
    _GroupNormFusedParams<Tp>                                              \
        <<<CUDA_2D_BLOCKS(N* G), CUDA_THREADS, 0, ctx->cuda_stream()>>>(   \
            N, G, D, mu, rsig, gamma, beta, scale, bias);                  \
    if (data_format == "NCHW") {                                           \
      _GroupNormForwardNCHW<Tx, Tp>                                        \
          <<<CUDA_2D_BLOCKS(N* C), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
              N, C, S, x, scale, bias, y);                                 \
    } else if (data_format == "NHWC") {                                    \
      _GroupNormForwardNHWC<Tx, Tp>                                        \
          <<<CUDA_2D_BLOCKS(N* C), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
              N, C, S, x, scale, bias, y);                                 \
    }                                                                      \
  }

#define DEFINE_GRAD_KERNEL_LAUNCHER(Tx, Tp)                                 \
  template <>                                                               \
  void GroupNormBackward<Tx, Tp, CUDAContext>(                              \
      const int N,                                                          \
      const int G,                                                          \
      const int D,                                                          \
      const int S,                                                          \
      const string& data_format,                                            \
      const Tx* x,                                                          \
      const Tp* mu,                                                         \
      const Tp* rsig,                                                       \
      const Tp* gamma,                                                      \
      const Tx* dy,                                                         \
      Tp* ds,                                                               \
      Tp* db,                                                               \
      Tx* dx,                                                               \
      Tp* dgamma,                                                           \
      Tp* dbeta,                                                            \
      CUDAContext* ctx) {                                                   \
    auto nthreads = N * G * D * S;                                          \
    if (data_format == "NCHW") {                                            \
      _GroupNormWGrad<Tx, Tp, StorageOrder::NCHW>                           \
          <<<CUDA_2D_BLOCKS(G* D), CUDA_THREADS, 0, ctx->cuda_stream()>>>(  \
              N, G, D, S, x, mu, rsig, dy, dgamma, dbeta);                  \
      _GroupNormInternalGrad<Tx, Tp, StorageOrder::NCHW>                    \
          <<<CUDA_2D_BLOCKS(N* G), CUDA_THREADS, 0, ctx->cuda_stream()>>>(  \
              N, G, D, S, x, gamma, dy, ds, db);                            \
      _GroupNormGrad<Tx, Tp, StorageOrder::NCHW>                            \
          <<<CUDA_BLOCKS(nthreads), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
              nthreads, G, D, S, x, mu, rsig, gamma, ds, db, dy, dx);       \
    } else if (data_format == "NHWC") {                                     \
      _GroupNormWGrad<Tx, Tp, StorageOrder::NHWC>                           \
          <<<CUDA_2D_BLOCKS(G* D), CUDA_THREADS, 0, ctx->cuda_stream()>>>(  \
              N, G, D, S, x, mu, rsig, dy, dgamma, dbeta);                  \
      _GroupNormInternalGrad<Tx, Tp, StorageOrder::NHWC>                    \
          <<<CUDA_2D_BLOCKS(N* G), CUDA_THREADS, 0, ctx->cuda_stream()>>>(  \
              N, G, D, S, x, gamma, dy, ds, db);                            \
      _GroupNormGrad<Tx, Tp, StorageOrder::NHWC>                            \
          <<<CUDA_BLOCKS(nthreads), CUDA_THREADS, 0, ctx->cuda_stream()>>>( \
              nthreads, G, D, S, x, mu, rsig, gamma, ds, db, dy, dx);       \
    }                                                                       \
  }

DEFINE_KERNEL_LAUNCHER(float, float);
DEFINE_GRAD_KERNEL_LAUNCHER(float, float);

#undef DEFINE_KERNEL_LAUNCHER
#undef DEFINE_GRAD_KERNEL_LAUNCHER

} // namespace kernel

} // namespace dragon

#endif // USE_CUDA
