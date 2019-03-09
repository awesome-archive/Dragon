#include "core/workspace.h"
#include "utils/op_kernel.h"
#include "utils/math_functions.h"
#include "operators/vision/roi_align_op.h"

namespace dragon {

template <class Context> template <typename T>
void ROIAlignOp<Context>::RunWithType() {
    auto* Xdata = Input(0).template data<T, Context>();
    auto* Rdata = Input(1).template data<float, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();

    kernel::ROIAlign(
        Input(0).dim(1), Input(0).dim(2), Input(0).dim(3),
            pool_h, pool_w, Input(1).dim(0),
                spatial_scale, sampling_ratio,
                    Xdata, Rdata, Ydata, ctx());
}

template <class Context>
void ROIAlignOp<Context>::RunOnDevice() {
    Output(0)->Reshape({
        Input(1).dim(0),    /*!   Number of RoIs  */
        Input(0).dim(1),    /*!   Channels        */
        pool_h,             /*!   Pooled height   */
        pool_w              /*!   Pooled width    */
    });

    if (XIsType(Input(0), float)) RunWithType<float>();
    else if (XIsType(Input(0), float16)) RunWithType<float16>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CPU(ROIAlign);
#ifdef WITH_CUDA
DEPLOY_CUDA(ROIAlign);
#endif
OPERATOR_SCHEMA(ROIAlign).NumInputs(2).NumOutputs(1);

template <class Context> template <typename T>
void ROIAlignGradientOp<Context>::RunWithType() {
    auto* dYdata = Input(-1).template data<T, Context>();
    auto* Rdata = Input(1).template data<float, Context>();
    auto* dXdata = Output(0)->template mutable_data<T, Context>();

    math::Set(Output(0)->count(), cast::to<T>(0.f), dXdata, ctx());

    kernel::ROIAlignGrad(
        Output(0)->dim(1), Output(0)->dim(2), Output(0)->dim(3),
            pool_h, pool_w, Input(1).dim(0),
                spatial_scale, sampling_ratio,
                    dYdata, Rdata, dXdata, ctx());
}

template <class Context>
void ROIAlignGradientOp<Context>::RunWithFloat16() {
    auto* dYdata = Input(-1).template data<float16, Context>();
    auto* Rdata = Input(1).template data<float, Context>();
    auto* dXdata = Output(0)->template mutable_data<float16, Context>();

    auto WSdata = ws()->template caches<float, Context>({
        Input(-1).count(), Output(0)->count() });

    math::Set(Output(0)->count(), 0.f, WSdata[1], ctx());
    kernel::TypeA2B(Input(-1).count(), dYdata, WSdata[0], ctx());

    kernel::ROIAlignGrad(
        Output(0)->dim(1), Output(0)->dim(2), Output(0)->dim(3),
            pool_h, pool_w, Input(1).dim(0),
                spatial_scale, sampling_ratio,
                    WSdata[0], Rdata, WSdata[1], ctx());

    kernel::TypeA2B(Output(0)->count(), WSdata[1], dXdata, ctx());
}

template <class Context>
void ROIAlignGradientOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(0));

    if (XIsType(Input(0), float)) RunWithType<float>();
    else if (XIsType(Input(0), float16)) RunWithFloat16();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CPU(ROIAlignGradient);
#ifdef WITH_CUDA
DEPLOY_CUDA(ROIAlignGradient);
#endif

OPERATOR_SCHEMA(ROIAlignGradient)
    .NumInputs(3).NumOutputs(1);

class GetROIAlignGradient final : public GradientMakerBase {
 public:
    GRADIENT_MAKER_CTOR(GetROIAlignGradient);
    vector<OperatorDef> MakeDefs() override {
        return SingleDef(def.type() + "Gradient", "",
            vector<string>({ I(0), I(1), GO(0) }),
            vector<string>({ GI(0) }));
    }
};

REGISTER_GRADIENT(ROIAlign, GetROIAlignGradient);

}  // namespace dragon