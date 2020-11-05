#include "dragon/operators/activation/drop_path_op.h"
#include "dragon/core/workspace.h"
#include "dragon/utils/math_functions.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

template <class Context>
template <typename T>
void DropPathOp<Context>::DoRunWithType() {
  auto &X = Input(0), *Y = Output(0, {0});
  if (phase() == "TEST") {
    Y->ReshapeLike(X)->CopyFrom(X, ctx());
  } else if (phase() == "TRAIN") {
    auto* mask = Buffer("mask")
                     ->Reshape({X.dim(0)})
                     ->template mutable_data<float, Context>();
    auto* scale = Buffer("scale")
                      ->Reshape({})
                      ->template mutable_data<float, CPUContext>();
    // Generate mask for each example
    math::RandomUniform(X.dim(0), 0.f, 1.f, mask, ctx());
    // Apply mask to the feature
    kernel::DropPath(
        X.dim(0),
        X.stride(0),
        1.f / (1.f - ratio()),
        X.template data<T, Context>(),
        mask,
        Y->ReshapeLike(X)->template mutable_data<T, Context>(),
        ctx());
  } else {
    LOG(FATAL) << "Unknown Phase: " << phase();
  }
}

template <class Context>
void DropPathOp<Context>::RunOnDevice() {
  DispatchHelper<FloatingTensorTypes>::Call(this, Input(0));
}

template <class Context>
template <typename T>
void DropPathGradientOp<Context>::DoRunWithType() {
  auto &dY = Input(0), *dX = Output(0);
  if (phase() == "TEST") {
    NOT_IMPLEMENTED;
  } else if (phase() == "TRAIN") {
    kernel::DropPath(
        dY.dim(0),
        dY.stride(0),
        1.f / (1.f - ratio()),
        dY.template data<T, Context>(),
        Buffer("mask")->template data<float, Context>(),
        dX->ReshapeLike(dY)->template mutable_data<T, Context>(),
        ctx());
  } else {
    LOG(FATAL) << "Unknown Phase: " << phase();
  }
}

template <class Context>
void DropPathGradientOp<Context>::RunOnDevice() {
  DispatchHelper<FloatingTensorTypes>::Call(this, Input(0));
}

DEPLOY_CPU_OPERATOR(DropPath);
#ifdef USE_CUDA
DEPLOY_CUDA_OPERATOR(DropPath);
#endif

DEPLOY_CPU_OPERATOR(DropPathGradient);
#ifdef USE_CUDA
DEPLOY_CUDA_OPERATOR(DropPathGradient);
#endif

OPERATOR_SCHEMA(DropPath)
    /* X */
    .NumInputs(1)
    /* Y */
    .NumOutputs(1)
    /* X => Y */
    .AllowInplace({{0, 0}});

OPERATOR_SCHEMA(DropPathGradient)
    /* dY */
    .NumInputs(1)
    /* dX */
    .NumOutputs(1)
    /* dY => dX */
    .AllowInplace({{0, 0}});

REGISTER_GRADIENT(DropPath, SimpleGradientMaker);

} // namespace dragon
