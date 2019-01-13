#include "core/workspace.h"
#include "utils/op_kernel.h"
#include "utils/math_functions.h"
#include "operators/vision/nn_resize_op.h"

namespace dragon {

template <class Context> template <typename T>
void NNResizeOp<Context>::RunWithType() {
    if (data_format == "NCHW") {
        n = Input(0).dim(0), c = Input(0).dim(1);
        h = Input(0).dim(2), w = Input(0).dim(3);
        out_h = Output(0)->dim(2), out_w = Output(0)->dim(3);
    } else if (data_format == "NHWC") {
        n = Input(0).dim(0), h = Input(0).dim(1);
        w = Input(0).dim(2), c = Input(0).dim(3);
        out_h = Output(0)->dim(1), out_w = Output(0)->dim(2);
    }
    auto* Xdata = Input(0).template data<T, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();

    kernel::NNResize(n, c, h, w, out_h, out_w,
        data_format, Xdata, Ydata, ctx());
}

template <class Context>
void NNResizeOp<Context>::RunOnDevice() {
    vector<int64_t> dims = Input(0).dims();
    if (dsize_desc.size() > 0 || dsize_value.size() > 0) {
        for (int i = 0; i < 2; i++)
            dims[spatial_axis + i] = dsize(i);
    } else if (!shape_like_desc.empty()) {
        Tensor* shape_like_tensor = ws()->GetTensor(shape_like_desc);
        for (int i = 0; i < 2; i++)
            dims[spatial_axis + i] = shape_like_tensor->dim(spatial_axis + i);
    } else {
        CHECK(fy != -1.f && fx != -1.f)
                << "\nThe fx and fy should be set.";
        dims[spatial_axis] = int(dims[spatial_axis] * fy);
        dims[spatial_axis + 1] = int(dims[spatial_axis + 1] * fx);
    }
    Output(0)->Reshape(dims);

    if (XIsType(Input(0), float)) RunWithType<float>();
    else if (XIsType(Input(0), float16)) RunWithType<float16>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CPU(NNResize);
#ifdef WITH_CUDA
DEPLOY_CUDA(NNResize);
#endif
OPERATOR_SCHEMA(NNResize).NumInputs(1).NumOutputs(1);

template <class Context> template <typename T>
void NNResizeGradientOp<Context>::RunWithType() {
    if (data_format == "NCHW") {
        n = Input(0).dim(0), c = Input(0).dim(1);
        h = Input(0).dim(2), w = Input(0).dim(3);
        out_h = Input(-1).dim(2), out_w = Input(-1).dim(3);
    } else if (data_format == "NHWC") {
        n = Input(0).dim(0), h = Input(0).dim(1);
        w = Input(0).dim(2), c = Input(0).dim(3);
        out_h = Input(-1).dim(1), out_w = Input(-1).dim(2);
    }
    auto* dYdata = Input(-1).template data<T, Context>();
    auto* dXdata = Output(0)->template mutable_data<T, Context>();

    math::Set<T, Context>(Output(0)->count(), 0, dXdata, ctx());

    kernel::NNResizeGrad(n, c, h, w, out_h, out_w,
        data_format, dYdata, dXdata, ctx());
}

template <class Context>
void NNResizeGradientOp<Context>::RunOnDevice() {
    Output(0)->ReshapeLike(Input(0));
    
    if (XIsType(Input(0), float)) RunWithType<float>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32" });
}

DEPLOY_CPU(NNResizeGradient);
#ifdef WITH_CUDA
DEPLOY_CUDA(NNResizeGradient);
#endif

OPERATOR_SCHEMA(NNResizeGradient)
    .NumInputs(2).NumOutputs(1);

REGISTER_GRADIENT(NNResize, SimpleGradientMaker);

}  // namespace dragon