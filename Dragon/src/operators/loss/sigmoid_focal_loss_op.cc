#include "core/workspace.h"
#include "utils/op_kernel.h"
#include "utils/math_functions.h"
#include "operators/loss/sigmoid_focal_loss_op.h"

namespace dragon {

#define DETERMINE_RUNTIME_ARGUMENTS(X) \
    axis = OperatorBase::Arg<int64_t>("axis", 1); \
    axis = axis < 0 ? axis + X.ndim() : axis; \
    CHECK(axis >= 0 && axis < X.ndim()) \
       << "\nExcepted the axis in [-" << X.ndim() << ", " << X.ndim() \
       << "), got " << OperatorBase::Arg<int64_t>("axis", 1) << ".";

template <class Context> template <typename Tx, typename Ty>
void SigmoidFocalLossOp<Context>::RunWithType() {
    auto* Xdata = Input(0).template data<Tx, Context>();
    auto* Tdata = Input(1).template data<Ty, Context>();
    auto* Ldata = losses.template mutable_data<Tx, Context>();
    auto* Fdata = flags.template mutable_data<int, Context>();

    kernel::SigmoidFocalLoss(
        outer_dim, axis_dim, inner_dim,
            pos_alpha, neg_alpha, gamma, neg_id,
                Xdata, Tdata, Ldata, Fdata, ctx());

    if (normalization == "UNIT") {
        vector<int64_t> output_dims = Input(0).dims();
        output_dims.erase(output_dims.begin() + axis);
        Output(0)->Reshape(output_dims);
        Output(0)->template CopyFrom<Context>(
            losses, ctx()); return;
    }

    double normalizer = 1.;
    if (normalization == "VALID") {
        normalizer = std::max(
            math::Sum(flags.count(),
                1.f, Fdata, ctx()), 1);
    } else if (normalization == "BATCH_SIZE") {
        normalizer = (float)Input(0).dim(0);
    } else if (normalization == "FULL") {
        normalizer = (float)(outer_dim * inner_dim);
    }

    Output(0)->Reshape(vector<int64_t>());
    auto* Ydata = Output(0)->template mutable_data<Tx, Context>();
    math::Sum(losses.count(), 1. / normalizer, Ldata, Ydata, ctx());
}

template <class Context>
void SigmoidFocalLossOp<Context>::RunOnDevice() {
    DETERMINE_RUNTIME_ARGUMENTS(Input(0));

    outer_dim = Input(0).count(0, axis);
    axis_dim = Input(0).dim(axis);
    inner_dim = Input(0).count(axis + 1);
    CHECK_EQ(outer_dim * inner_dim, Input(1).count())
        << "\nNumber of predictions must match the number of labels.";

    losses.ReshapeLike(Input(0));
    flags.ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) {
        if (XIsType(Input(1), float)) RunWithType<float, float>();
        else if (XIsType(Input(1), int64_t)) RunWithType<float, int64_t>();
        else LOG(FATAL) << DTypeHelper(Input(1), { "float32", "int64" });
    } else LOG(FATAL) << DTypeHelper(Input(0), { "float32" });
}

DEPLOY_CPU(SigmoidFocalLoss);
#ifdef WITH_CUDA
DEPLOY_CUDA(SigmoidFocalLoss);
#endif
OPERATOR_SCHEMA(SigmoidFocalLoss).NumInputs(2).NumOutputs(1);

template <class Context> template <typename Tx, typename Ty>
void SigmoidFocalLossGradientOp<Context>::RunWithType() {
    auto* Xdata = Input(0).template data<Tx, Context>();
    auto* Tdata = Input(1).template data<Ty, Context>();
    auto* dXdata = Output(0)->template mutable_data<Tx, Context>();
    auto* Fdata = flags.template mutable_data<int, Context>();

    kernel::SigmoidFocalLossGrad(
        outer_dim, axis_dim, inner_dim,
            pos_alpha, neg_alpha, gamma, neg_id,
                Xdata, Tdata, dXdata, Fdata, ctx());

    if (normalization == "UNIT") {
        auto* dYdata = Input(-1).template data<Tx, Context>();
        math::Mul(Output(0)->count(),
            dYdata, dXdata, dXdata, ctx()); return;
    }

    double normalizer = 1.;
    if (normalization == "VALID") {
        normalizer = std::max(
            math::Sum(flags.count(),
                1.f, Fdata, ctx()), 1);
    } else if (normalization == "BATCH_SIZE") {
        normalizer = Input(0).dim(0);
    } else if (normalization == "FULL") {
        normalizer = Input(0).count();
    }

    auto* dYdata = Input(-1).template data<Tx, Context>();
    Tx dYHost; ctx()->template Copy
        <Tx, CPUContext, Context>(
            1, &dYHost, dYdata);
    ctx()->FinishDeviceCompution();

    math::Scale(
        Output(0)->count(),
            dYHost / normalizer,
                dXdata, dXdata, ctx());
}

template <class Context>
void SigmoidFocalLossGradientOp<Context>::RunOnDevice() {
    DETERMINE_RUNTIME_ARGUMENTS(Input(0));

    outer_dim = Input(0).count(0, axis);
    axis_dim = Input(0).dim(axis);
    inner_dim = Input(0).count(axis + 1);

    Output(0)->ReshapeLike(Input(0));
    flags.ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) {
        if (XIsType(Input(1), float)) RunWithType<float, float>();
        else if (XIsType(Input(1), int64_t)) RunWithType<float, int64_t>();
        else LOG(FATAL) << DTypeHelper(Input(1), { "float32", "int64" });
    } else LOG(FATAL) << DTypeHelper(Input(0), { "float32" });
}

DEPLOY_CPU(SigmoidFocalLossGradient);
#ifdef WITH_CUDA
DEPLOY_CUDA(SigmoidFocalLossGradient);
#endif

OPERATOR_SCHEMA(SigmoidFocalLossGradient)
    .NumInputs(3).NumOutputs(1);

class GetSigmoidFocalLossGradient
    final : public GradientMakerBase {
 public:
    GRADIENT_MAKER_CTOR(GetSigmoidFocalLossGradient);
    vector<OperatorDef> MakeDefs() override {
        return SingleDef(def.type() + "Gradient", "",
            vector<string>({ I(0), I(1), GO(0) }),
            vector<string>({ GI(0) }));
    }
};

REGISTER_GRADIENT(
    SigmoidFocalLoss,
    GetSigmoidFocalLossGradient
);

#undef DETERMINE_RUNTIME_ARGUMENTS

}  // namespace dragon