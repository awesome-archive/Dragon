#include "utils/math_functions.h"
#include "utils/op_kernel.h"
#include "operators/activation/tanh_op.h"

namespace dragon {

template <class Context> template <typename T>
void TanhOp<Context>::RunWithType() {
    auto* Xdata = Input(0).template data<T, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();
    kernel::Tanh(Output(0)->count(), Xdata, Ydata, ctx());
}

template <class Context>
void TanhOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) RunWithType<float>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32" });
}

DEPLOY_CPU(Tanh);
#ifdef WITH_CUDA
DEPLOY_CUDA(Tanh);
#endif
OPERATOR_SCHEMA(Tanh).NumInputs(1).NumOutputs(1).Inplace({ { 0, 0 } });

template <class Context> template <typename T>
void TanhGradientOp<Context>::RunWithType() {
    auto* Ydata = Input(0).template data<T, Context>();
    auto* dYdata = Input(1).template data<T, Context>();
    auto* dXdata = Output(0)->template mutable_data<T, Context>();
    kernel::TanhGrad(Output(0)->count(), dYdata, Ydata, dXdata, ctx());
}

template <class Context>
void TanhGradientOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) RunWithType<float>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32" });
}

DEPLOY_CPU(TanhGradient);
#ifdef WITH_CUDA
DEPLOY_CUDA(TanhGradient);
#endif

OPERATOR_SCHEMA(TanhGradient)
    .NumInputs(2).NumOutputs(1)
    .Inplace({ { 1, 0 } });

REGISTER_GRADIENT(Tanh, InplaceGradientMaker);

}  // namespace dragon