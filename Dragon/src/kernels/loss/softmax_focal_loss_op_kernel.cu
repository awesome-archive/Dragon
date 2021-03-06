#ifdef WITH_CUDA

#include "core/context_cuda.h"
#include "utils/op_kernel.h"

namespace dragon {

namespace kernel {

/*! SoftmaxFocalLoss <Tx = ?, Ty = ?, Device = CUDA> */

template <typename Tx, typename Ty>
__global__ void _SoftmaxFocalLoss(
    const int               count,
    const int               axis_dim,
    const int               inner_dim,
    const int               num_ignores,
    const float             pos_alpha,
    const float             neg_alpha,
    const float             gamma,
    const int               neg_id,
    const Tx*               prob,
    const Ty*               labels,
    const int*              ignores,
    Tx*                     losses,
    int*                    flags) {
    CUDA_1D_KERNEL_LOOP(idx, count) {
        const int oix = idx / inner_dim;
        const int iix = idx % inner_dim;
        const int label = labels[oix * inner_dim + iix];
        int k;
        for (k = 0; k < num_ignores; k++) {
            if (label == ignores[k]) {
                losses[idx] = flags[idx] = 0;
                break;
            }
        }
        if (k == num_ignores) {
            const int t = (oix * axis_dim + label) * inner_dim + iix;
            Tx scale = pow(1 - prob[t], gamma);
            scale = label > neg_id ?
                pos_alpha * scale : neg_alpha * scale;
            losses[idx] = -scale * log(max(prob[t], FLT_MIN));
            flags[idx] = label > neg_id ? 1 : 0;
        }
    }
}

/*! SoftmaxFocalLoss <Tx = float32, Ty = float32, Device = CUDA> */

template <> void SoftmaxFocalLoss<float, float, CUDAContext>(
    const int               outer_dim,
    const int               axis_dim,
    const int               inner_dim,
    const int               num_ignores,
    const float             pos_alpha,
    const float             neg_alpha,
    const float             gamma,
    const int               neg_id,
    const float*            prob,
    const float*            labels,
    const int*              ignores,
    float*                  losses,
    int*                    flags,
    CUDAContext*            ctx) {
    const auto num_preds = outer_dim * inner_dim;
    _SoftmaxFocalLoss<float, float>
        << < CUDA_BLOCKS(num_preds), CUDA_THREADS,
             0, ctx->cuda_stream() >> >
        (num_preds, axis_dim, inner_dim, num_ignores,
            pos_alpha, neg_alpha, gamma, neg_id,
                prob, labels, ignores, losses, flags);
}

/*! SoftmaxFocalLoss <Tx = float32, Ty = int64, Device = CUDA> */

template <> void SoftmaxFocalLoss<float, int64_t, CUDAContext>(
    const int               outer_dim,
    const int               axis_dim,
    const int               inner_dim,
    const int               num_ignores,
    const float             pos_alpha,
    const float             neg_alpha,
    const float             gamma,
    const int               neg_id,
    const float*            prob,
    const int64_t*          labels,
    const int*              ignores,
    float*                  losses,
    int*                    flags,
    CUDAContext*            ctx) {
    const auto num_preds = outer_dim * inner_dim;
    _SoftmaxFocalLoss<float, int64_t>
        << < CUDA_BLOCKS(num_preds), CUDA_THREADS,
             0, ctx->cuda_stream() >> >
        (num_preds, axis_dim, inner_dim, num_ignores,
            pos_alpha, neg_alpha, gamma, neg_id,
                prob, labels, ignores, losses, flags);
}

/*! SoftmaxFocalLossGrad <Tx = ?, Ty = ?, Device = CUDA> */

template <typename Tx, typename Ty>
__global__ void _SoftmaxFocalLossGrad(
    const int               count,
    const int               axis_dim,
    const int               inner_dim,
    const int               num_ignores,
    const float             pos_alpha,
    const float             neg_alpha,
    const float             gamma,
    const int               neg_id,
    const Tx*               prob,
    const Ty*               labels,
    const int*              ignores,
    Tx*                     dx,
    int*                    flags) {
    CUDA_1D_KERNEL_LOOP(idx, count) {
        const int oix = idx / inner_dim;
        const int iix = idx % inner_dim;
        const int label = labels[oix * inner_dim + iix];
        int k;
        for (k = 0; k < num_ignores; k++)
            if (label == ignores[k]) break;
        if (k != num_ignores) {
            for (int c = 0; c < axis_dim; c++)
                dx[(oix * axis_dim + c) * inner_dim + iix] = (Tx)0;
            flags[idx] = 0;
        } else {
            const int t = (oix * axis_dim + label) * inner_dim + iix;
            Tx onemp = 1 - prob[t];
            // Unstable if gamma is 0
            Tx grad = -gamma * pow(onemp, gamma - 1)
                             * log(max(prob[t], FLT_MIN))
                             * prob[t] + pow(onemp, gamma);
            grad = label > neg_id ?
                pos_alpha * grad : neg_alpha * grad;
            for (int c = 0; c < axis_dim; c++) {
                const int i = (oix * axis_dim + c) * inner_dim + iix;
                if (c == label) {
                    dx[i] = grad * (prob[t] - 1);
                } else {
                    dx[i] = grad * prob[i];
                }
            }
            flags[idx] = label > neg_id ? 1 : 0;
        }
    }
}

/*! SoftmaxFocalLossGrad <Tx = float32, Ty = float32, Device = CUDA> */

template<> void SoftmaxFocalLossGrad<float, float, CUDAContext>(
    const int               outer_dim,
    const int               axis_dim,
    const int               inner_dim,
    const int               num_ignores,
    const float             pos_alpha,
    const float             neg_alpha,
    const float             gamma,
    const int               neg_id,
    const float*            prob,
    const float*            labels,
    const int*              ignores,
    float*                  dx,
    int*                    flags,
    CUDAContext*            ctx) {
    const int num_preds = outer_dim * inner_dim;
    _SoftmaxFocalLossGrad<float, float>
        << < CUDA_BLOCKS(num_preds), CUDA_THREADS,
             0, ctx->cuda_stream() >> >
        (num_preds, axis_dim, inner_dim, num_ignores,
            pos_alpha, neg_alpha, gamma, neg_id,
                prob, labels, ignores, dx, flags);
}

/*! SoftmaxFocalLossGrad <Tx = float32, Ty = int64, Device = CUDA> */

template<> void SoftmaxFocalLossGrad<float, int64_t, CUDAContext>(
    const int               outer_dim,
    const int               axis_dim,
    const int               inner_dim,
    const int               num_ignores,
    const float             pos_alpha,
    const float             neg_alpha,
    const float             gamma,
    const int               neg_id,
    const float*            prob,
    const int64_t*          labels,
    const int*              ignores,
    float*                  dx,
    int*                    flags,
    CUDAContext*            ctx) {
    const int num_preds = outer_dim * inner_dim;
    _SoftmaxFocalLossGrad<float, int64_t>
        << < CUDA_BLOCKS(num_preds), CUDA_THREADS,
             0, ctx->cuda_stream() >> >
        (num_preds, axis_dim, inner_dim, num_ignores,
            pos_alpha, neg_alpha, gamma, neg_id,
                prob, labels, ignores, dx, flags);
}

}  // namespace kernel

}  // namepsace dragon

#endif  // WITH_CUDA