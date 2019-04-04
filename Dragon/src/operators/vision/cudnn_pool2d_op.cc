#ifdef WITH_CUDNN

#include "operators/vision/pool_op.h"

namespace dragon {

template <class Context> template <typename T>
void CuDNNPool2dOp<Context>::RunWithType() {
    cudnnSetTensor4dDesc<T>(&input_desc, this->data_format, &Input(0));
    cudnnSetTensor4dDesc<T>(&output_desc, this->data_format, Output(0));
#if CUDNN_VERSION_MIN(5, 0, 0)
    CUDNN_CHECK(cudnnSetPooling2dDescriptor(
        pool_desc,
        pool_mode,
        CUDNN_PROPAGATE_NAN,
        this->kernel_shape[0], this->kernel_shape[1],
        this->pad_l[0], this->pad_l[1],
        this->stride[0], this->stride[1]));
#else
    CUDNN_CHECK(cudnnSetPooling2dDescriptor_v4(
        pool_desc,
        pool_mode,
        CUDNN_PROPAGATE_NAN,
        this->kernel_shape[0], this->kernel_shape[1],
        this->pad_l[0], this->pad_l[1],
        this->stride[0], this->stride[1]));
#endif
    auto* Xdata = Input(0).template data<T, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();

    CUDNN_CHECK(cudnnPoolingForward(
        ctx()->cudnn_handle(), pool_desc,
        CUDNNType<T>::one, input_desc, Xdata,
        CUDNNType<T>::zero, output_desc, Ydata));
}

template <class Context>
void CuDNNPool2dOp<Context>::RunOnDevice() {
    Pool2dOp<Context>::Reshape();

    if (XIsType(Input(0), float)) RunWithType<float>();
    else if (XIsType(Input(0), float16)) RunWithType<float16>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CUDNN(Pool2d);

template <class Context> template <typename T>
void CuDNNPool2dGradientOp<Context>::RunWithType() {
    cudnnSetTensor4dDesc<T>(&input_desc, this->data_format, &Input(-1));
    cudnnSetTensor4dDesc<T>(&output_desc, this->data_format, Output(0));
#if CUDNN_VERSION_MIN(5, 0, 0)
    CUDNN_CHECK(cudnnSetPooling2dDescriptor(
        pool_desc,
        pool_mode,
        CUDNN_PROPAGATE_NAN,
        this->kernel_shape[0], this->kernel_shape[1],
        this->pad_l[0], this->pad_l[1],
        this->stride[0], this->stride[1]));
#else
    CUDNN_CHECK(cudnnSetPooling2dDescriptor_v4(
        pool_desc,
        pool_mode,
        CUDNN_PROPAGATE_NAN,
        this->kernel_shape[0], this->kernel_shape[1],
        this->pad_l[0], this->pad_l[1],
        this->stride[0], this->stride[1]));
#endif
    auto* dYdata = Input(-1).template data<T, Context>();
    auto* Xdata = Input(0).template data<T, Context>();
    auto* Ydata = Input(1).template data<T, Context>();
    auto* dXdata = Output(0)->template mutable_data<T, Context>();

    CUDNN_CHECK(cudnnPoolingBackward(
        ctx()->cudnn_handle(), pool_desc,
        CUDNNType<T>::one, input_desc, Ydata,
        input_desc, dYdata, output_desc, Xdata,
        CUDNNType<T>::zero, output_desc, dXdata));
}

template <class Context>
void CuDNNPool2dGradientOp<Context>::RunOnDevice() {
    Pool2dGradientOp<Context>::Reshape();

    if (XIsType(Input(0), float)) RunWithType<float>();
    else if (XIsType(Input(0), float16)) RunWithType<float16>();
    else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CUDNN(Pool2dGradient);

}  // namespace dragon

#endif  // WITH_CUDNN