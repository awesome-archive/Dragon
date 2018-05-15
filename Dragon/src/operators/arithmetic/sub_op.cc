#include "operators/arithmetic/sub_op.h"
#include "core/workspace.h"
#include "utils/math_functions.h" 

namespace dragon {

template <class Context> template <typename T>
void SubOp<Context>::EltwiseRunWithType() {
    auto* X1data = Input(0).template data<T, Context>();
    auto* X2data = Input(1).template data<T, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();
    math::Sub<T, Context>(Input(0).count(), X1data, X2data, Ydata);
}

template <class Context> template <typename T>
void SubOp<Context>::BroadcastRunWithType(int type) {
    TIndex outer_dim, inner_dim;
    auto* X1data = Input(0).template data<T, Context>();
    auto* X2data = Input(1).template data<T, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();
    ctx().template Copy<T, Context, Context>(Input(0).count(), Ydata, X1data);

    if (type == 0 || type == 1) {
        if (type == 0) {
            outer_dim = Input(0).count();
            inner_dim = 1;
        } else {
            outer_dim = Input(0).count(0, Input(0).axis(-1));
            inner_dim = Input(0).dim(-1);
        }
        INIT_MULTIPLIER(bcast_multiplier, outer_dim);
        auto* BMul_data = bcast_multiplier->template data<T, Context>();
        math::Gemm<T, Context>(CblasNoTrans, CblasNoTrans, outer_dim, inner_dim, 1,
            -1.0, bcast_multiplier->template data<T, Context>(), X2data, 1.0, Ydata);
    } 
    else if (type == 2) {
        outer_dim = Input(0).dim(0);
        inner_dim = Input(0).count(1);
        INIT_MULTIPLIER(bcast_multiplier, inner_dim);
        auto* BMul_data = bcast_multiplier->template data<T, Context>();
        math::Gemm<T, Context>(CblasNoTrans, CblasNoTrans, outer_dim, inner_dim, 1,
            -1.0, X2data, bcast_multiplier->template data<T, Context>(), 1.0, Ydata);
    }
}

template <class Context>
void SubOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) {
        if (Input(0).dims() == Input(1).dims()) 
            EltwiseRunWithType<float>();
        else if (Input(0).dim(0) == Input(1).dim(0) && Input(1).count(1) == 1) 
            BroadcastRunWithType<float>(2);
        else if (Input(0).dim(-1) == Input(1).dim(-1) && 
                 Input(1).count(0, Input(1).axis(-1)) == 1)  
            BroadcastRunWithType<float>(1);
        else if (Input(1).ndim() == 1 && Input(1).dim(0) == 1)
            BroadcastRunWithType<float>(0);
        else LOG(FATAL) << "Could not be broadcast together with shapes "
                        << Input(0).dim_string() << "  " << Input(1).dim_string(); 
    } else if (XIsType(Input(0), float16)) {
        if (Input(0).dims() == Input(1).dims())
            EltwiseRunWithType<float16>();
        else if (Input(0).dim(0) == Input(1).dim(0) && Input(1).count(1) == 1)
            BroadcastRunWithType<float16>(2);
        else if (Input(0).dim(-1) == Input(1).dim(-1) &&
            Input(1).count(0, Input(1).axis(-1)) == 1)
            BroadcastRunWithType<float16>(1);
        else if (Input(1).ndim() == 1 && Input(1).dim(0) == 1)
            BroadcastRunWithType<float16>(0);
        else LOG(FATAL) << "Could not be broadcast together with shapes "
                        << Input(0).dim_string() << "  " << Input(1).dim_string();
    } else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CPU(Sub);
#ifdef WITH_CUDA
DEPLOY_CUDA(Sub);
#endif
OPERATOR_SCHEMA(Sub).NumInputs(2).NumOutputs(1).Inplace({ { 0, 0 }, { 1, 0 } });

template <class Context> template <typename T>
void SubGradientOp<Context>::EltwiseRunWithType() {
    auto* dYdata = Input(-1).template data<T, Context>();
    if (Output(1)->name() != "ignore") {
        auto* dX2data = Output(1)->template mutable_data<T, Context>();
        math::Scale<T, Context>(Output(1)->count(), -1.0, dYdata, dX2data);
    }
    if (Output(0)->name() != "ignore") {
        auto* dX1data = Output(0)->template mutable_data<T, Context>();
        ctx().template Copy<T, Context, Context>(Output(0)->count(), dX1data, dYdata);
    }
}

template <class Context> template <typename T>
void SubGradientOp<Context>::BroadcastRunWithType(int type) {
    TIndex outer_dim, inner_dim;
    auto* dYdata = Input(-1).template data<T, Context>();

    if (Output(1)->name() != "ignore") {
        auto* dX2data = Output(1)->template mutable_data<T, Context>();
        if (type == 0 || type == 1) {
            if (type == 0) {
                outer_dim = Input(-1).count();
                inner_dim = 1;
            } else {
                outer_dim = Input(-1).count(0, Input(-1).axis(-1));
                inner_dim = Input(-1).dim(-1);
            }
            INIT_MULTIPLIER(bcast_multiplier, outer_dim);
            auto* BMul_data = bcast_multiplier->template data<T, Context>();
            math::Gemv<T, Context>(CblasTrans, outer_dim, inner_dim,
                                   -1.0, dYdata, BMul_data, 0.0, dX2data);
        }
        else if (type == 2) {
            outer_dim = Input(-1).dim(0);
            inner_dim = Input(-1).count(1);
            INIT_MULTIPLIER(bcast_multiplier, inner_dim);
            auto* BMul_data = bcast_multiplier->template data<T, Context>();
            math::Gemv<T, Context>(CblasNoTrans, outer_dim, inner_dim,
                                   -1.0, dYdata, BMul_data, 0.0, dX2data);
        }
    }

    if (Output(0)->name() != "ignore") {
        auto* dX1data = Output(0)->template mutable_data<T, Context>();
        ctx().template Copy<T, Context, Context>(Output(0)->count(), dX1data, dYdata);
    }
}

template <class Context>
void SubGradientOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(-1));
    Output(1)->ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) {
        if (Input(-1).dims() == Input(0).dims()) 
            EltwiseRunWithType<float>();
        else if (Input(-1).dim(0) == Input(0).dim(0) && Input(0).count(1) == 1)
            BroadcastRunWithType<float>(2);
        else if (Input(-1).dim(-1) == Input(0).dim(-1) &&
                 Input(0).count(0, Input(0).axis(-1)) == 1)
            BroadcastRunWithType<float>(1);
        else if (Input(0).ndim() == 1 && Input(0).dim(0) == 1)
            BroadcastRunWithType<float>(0);
        else LOG(FATAL) << "Could not be broadcast together with shapes "
                        << Input(-1).dim_string() << "  " << Input(0).dim_string();
    } else if (XIsType(Input(0), float16)) {
        if (Input(-1).dims() == Input(0).dims()) 
            EltwiseRunWithType<float16>();
        else if (Input(-1).dim(0) == Input(0).dim(0) && Input(0).count(1) == 1)
            BroadcastRunWithType<float16>(2);
        else if (Input(-1).dim(-1) == Input(0).dim(-1) &&
                 Input(0).count(0, Input(0).axis(-1)) == 1)
            BroadcastRunWithType<float16>(1);
        else if (Input(0).ndim() == 1 && Input(0).dim(0) == 1)
            BroadcastRunWithType<float16>(0);
        else LOG(FATAL) << "Could not be broadcast together with shapes "
                        << Input(-1).dim_string() << "  " << Input(0).dim_string();
    } else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CPU(SubGradient);
#ifdef WITH_CUDA
DEPLOY_CUDA(SubGradient);
#endif
OPERATOR_SCHEMA(SubGradient).NumInputs(2).NumOutputs(2).Inplace({ { 1, 0 } });

class GetSubGradient : public GradientMakerBase {
 public:
    GRADIENT_MAKER_CTOR(GetSubGradient);
    vector<OperatorDef> MakeDefs() override {
        return SingleDef(def.type() + "Gradient", "",
            vector<string> {I(1), GO(0)},
            vector<string> {GI(0), GI(1)});
    }
};
REGISTER_GRADIENT(Sub, GetSubGradient);

}    // namespace dragon