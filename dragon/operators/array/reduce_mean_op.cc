#include "dragon/core/workspace.h"
#include "dragon/operators/array/reduce_ops.h"
#include "dragon/utils/math_functions.h"
#include "dragon/utils/op_kernels.h"

namespace dragon {

template <class Context>
template <typename T>
void ReduceMeanOp<Context>::DoRunWithType() {
  auto &X = Input(0), *Y = Output(0);

  // Determine the reduce axes
  vec64_t Y_dims(X.dims()), Y_shape;
  vec32_t X_dims(Y_dims.begin(), Y_dims.end());
  vec32_t reduce_axes(axes_.begin(), axes_.end());
  if (axes_.empty()) {
    reduce_axes.resize(X.ndim());
    for (int i = 0; i < X.ndim(); ++i)
      reduce_axes[i] = i;
  }
  for (int i = 0; i < reduce_axes.size(); ++i) {
    auto axis = reduce_axes[i];
    reduce_axes[i] = axis = axis < 0 ? axis + X.ndim() : axis;
    CHECK(axis >= 0 && axis < X.ndim())
        << "\nExcepted the axis in [-" << X.ndim() << ", " << X.ndim()
        << "), got " << axis << ".";
    Y_dims[axis] = 1;
  }

  // Squeeze the output shape if necessary
  for (int i = 0; i < X.ndim(); ++i) {
    if (keep_dims_ || Y_dims[i] != 1) Y_shape.push_back(Y_dims[i]);
  }

  // Store for the gradient calculation
  Buffer("Y_dims")->template CopyFrom<int64_t>(Y_dims);

  if (X.count() == 1) {
    Y->Reshape(Y_shape)->CopyFrom(X, ctx());
  } else {
    Y->Reshape(Y_shape);
    math::ReduceSum(
        X_dims.size(),
        X_dims.data(),
        reduce_axes.size(),
        reduce_axes.data(),
        1.f / ((float)X.count() / (float)Y->count()),
        X.template data<T, Context>(),
        Y->Reshape(Y_shape)->template mutable_data<T, Context>(),
        ctx());
  }
}

template <class Context>
void ReduceMeanOp<Context>::RunOnDevice() {
  STORE_INPUT_SPEC(0);
  DispatchHelper<NumericalTensorTypes>::Call(this, Input(0));
}

template <class Context>
template <typename T>
void ReduceMeanGradientOp<Context>::DoRunWithType() {
  auto &dY = Input(0), *dX = Output(0);

  if (dX->count() == 1) {
    dX->CopyFrom(dY, ctx());
  } else {
    vec64_t Y_dims;
    Buffer("Y_dims")->template CopyTo<int64_t>(Y_dims);
    kernel::ReduceSumGrad(
        dX->ndim(),
        dX->dims().data(),
        Y_dims.data(),
        Tensor(Y_dims).strides().data(),
        1.f / ((float)dX->count() / (float)dY.count()),
        dY.template data<T, Context>(),
        dX->template mutable_data<T, Context>(),
        ctx());
  }
}

template <class Context>
void ReduceMeanGradientOp<Context>::RunOnDevice() {
  Output(0)->ReshapeLike(RESTORE_INPUT_SPEC(0));
  DispatchHelper<FloatingTensorTypes>::Call(this, Input(0));
}

DEPLOY_CPU_OPERATOR(ReduceMean);
#ifdef USE_CUDA
DEPLOY_CUDA_OPERATOR(ReduceMean);
#endif

DEPLOY_CPU_OPERATOR(ReduceMeanGradient);
#ifdef USE_CUDA
DEPLOY_CUDA_OPERATOR(ReduceMeanGradient);
#endif

OPERATOR_SCHEMA(ReduceMean)
    /* X */
    .NumInputs(1)
    /* Y */
    .NumOutputs(1);

OPERATOR_SCHEMA(ReduceMeanGradient)
    /* dY */
    .NumInputs(1)
    /* dX */
    .NumOutputs(1);

REGISTER_GRADIENT(ReduceMean, SimpleGradientMaker);

} // namespace dragon
