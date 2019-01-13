#include "utils/math_functions.h"
#include "utils/op_kernel.h"
#include "operators/activation/elu_op.h"

namespace dragon {

template <class Context> template <typename T>
void EluOp<Context>::RunWithType() {
    auto* Xdata = Input(0).template data<T, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();
    kernel::Elu(Output(0)->count(), alpha, Xdata, Ydata, ctx());
}

template <class Context>
void EluOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) RunWithType<float>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32" });
}

DEPLOY_CPU(Elu);
#ifdef WITH_CUDA
DEPLOY_CUDA(Elu);
#endif
OPERATOR_SCHEMA(Elu).NumInputs(1).NumOutputs(1).Inplace({ { 0, 0 } });

template <class Context> template <typename T>
void EluGradientOp<Context>::RunWithType() {
    auto* Ydata = Input(0).template data<T, Context>();
    auto* dYdata = Input(1).template data<T, Context>();
    auto* dXdata = Output(0)->template mutable_data<T, Context>();

    kernel::EluGrad(Output(0)->count(),
        alpha, dYdata, Ydata, dXdata, ctx());
}

template <class Context>
void EluGradientOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) RunWithType<float>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32" });
}

DEPLOY_CPU(EluGradient);
#ifdef WITH_CUDA
DEPLOY_CUDA(EluGradient);
#endif

OPERATOR_SCHEMA(EluGradient)
    .NumInputs(2).NumOutputs(1)
    .Inplace({ { 1, 0 }});

REGISTER_GRADIENT(Elu, InplaceGradientMaker);

}  // namespace dragon