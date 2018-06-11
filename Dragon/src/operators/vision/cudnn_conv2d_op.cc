#ifdef WITH_CUDNN

#include "operators/vision/conv_op.h"
#include "core/workspace.h"
#include "utils/filler.h"
#include "utils/op_kernel.h"

namespace dragon {

#define WORKSPACE_LIMIT_BYTES 64 * 1024 * 1024 // 64MB

template <class Context> template <typename T>
void CuDNNConv2dOp<Context>::ResetDesc() {
#if CUDNN_VERSION_MIN(5, 0, 0)
    CUDNN_CHECK(cudnnSetFilter4dDescriptor(filter_desc,
                                    CUDNNType<T>::type,
                                                format,
                        this->num_output / cudnn_group,
                          this->channels / this->group,
          this->kernel_size[0], this->kernel_size[1]));
#else
    CUDNN_CHECK(cudnnSetFilter4dDescriptor_v4(filter_desc,
                                       CUDNNType<T>::type,
                                                   format,
                           this->num_output / cudnn_group,
                             this->channels / this->group,
             this->kernel_size[0], this->kernel_size[1]));
#endif

    //  determine the input & output shape
    input_dims = Input(0).dims();
    cudnnSetTensor4dDescWithGroup<T>(&input_desc, this->data_format, Input(0).dims(), cudnn_group);
    cudnnSetTensor4dDescWithGroup<T>(&output_desc, this->data_format, Output(0)->dims(), cudnn_group);

    //  determine the bias shape
    if (HasBias()) {
        bias_offset = this->num_output / cudnn_group;
        if (this->data_format == "NCHW") {
            cudnnSetTensor4dDesc<T>(&bias_desc, this->data_format, vector<TIndex>({ 1, bias_offset, 1, 1 }));
        } else if (this->data_format == "NHWC") {
            cudnnSetTensor4dDesc<T>(&bias_desc, this->data_format, vector<TIndex>({ 1, 1, 1, bias_offset }));
        }
    }

    //  determine the misc
    if (this->data_format == "NCHW") {
        this->x_offset = Input(0).count(1) / cudnn_group;
        this->y_offset = Output(0)->count(1) / cudnn_group;
    }  else if (this->data_format == "NHWC") {
        this->x_offset = Input(0).dim(-1) / cudnn_group;
        this->y_offset = Output(0)->dim(-1) / cudnn_group;
    }

    CUDNN_CHECK(cudnnGetConvolutionForwardAlgorithm(handle[0],
                                                   input_desc,
                                                  filter_desc,
                                                    conv_desc,
                                                  output_desc,
                CUDNN_CONVOLUTION_FWD_SPECIFY_WORKSPACE_LIMIT,
                                        WORKSPACE_LIMIT_BYTES,
                                                  &fwd_algo));

    CUDNN_CHECK(cudnnGetConvolutionForwardWorkspaceSize(handle[0],
                                                       input_desc,
                                                      filter_desc,
                                                        conv_desc,
                                                      output_desc,
                                                         fwd_algo,
                                       &workspace_fwd_data_size));
    if (workspace_fwd_data_size == 0) workspace_fwd_data_size += 1;
}


template <class Context> template <typename T>
void CuDNNConv2dOp<Context>::RunWithType() {
    if (Input(0).dims() != input_dims) ResetDesc<T>();
    Tensor* buffer = ws()->GetBuffer();
    buffer->Reshape(vector<TIndex>(1, cudnn_group * workspace_fwd_data_size));

    auto* Xdata = Input(0).template data<T, Context>();
    auto* Ydata = Output(0)->template mutable_data<T, Context>();
    TENSOR_FILL(Input(1), this->weight_shape);
    auto* Wdata = Input(1).template data<T, Context>();
    if (HasBias()) TENSOR_FILL(Input(2), this->bias_shape);

    for (int g = 0; g < cudnn_group; g++) {
        auto* workspace = buffer->template mutable_data<char, Context>();
        CUDNN_CHECK(cudnnConvolutionForward(handle[g],
                        CUDNNType<T>::one, input_desc, Xdata + this->x_offset * g,
                                     filter_desc, Wdata + this->weight_offset * g,
                                                                        conv_desc,
                                                                         fwd_algo,
                 workspace + g * workspace_fwd_data_size, workspace_fwd_data_size,
                    CUDNNType<T>::zero, output_desc, Ydata + this->y_offset * g));
        if (HasBias()) {
            auto* bias = Input(2).template data<T, Context>();
            CUDNN_CHECK(cudnnAddTensor(handle[g],
                       CUDNNType<T>::one, bias_desc, bias + this->bias_offset * g,
                     CUDNNType<T>::one, output_desc, Ydata + this->y_offset * g));
        }
    }
    kernel::Empty<T, Context>();
    ws()->ReleaseBuffer(buffer);
}

template <class Context>
void CuDNNConv2dOp<Context>::RunOnDevice() {
#if CUDNN_VERSION_MAX(6, 0, 0)
    for (int i = 0; i < this->dilation.size(); i++)
        if (this->dilation[i] != 1) return Conv2dOp<Context>::RunOnDevice();
#endif
    Conv2dOp<Context>::Reshape();

    if (XIsType(Input(0), float)) {
#if CUDNN_VERSION_MIN(6, 0, 0)
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                         this->dilation[0], this->dilation[1],
                                      CUDNN_CROSS_CORRELATION,
                                           CUDNN_DATA_FLOAT));
#else
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                                                         1, 1,
                                    CUDNN_CROSS_CORRELATION));
#endif
#if CUDNN_VERSION_MIN(7, 0, 0)
        CUDNN_CHECK(cudnnSetConvolutionGroupCount(conv_desc, this->group));
        if (enable_tensor_core)
            CUDNN_CHECK(cudnnSetConvolutionMathType(conv_desc, CUDNN_TENSOR_OP_MATH));
#endif
        RunWithType<float>();
    } else if (XIsType(Input(0), float16)) {
#ifdef WITH_CUDA_FP16
#if CUDNN_VERSION_MIN(6, 0, 0)
        compute_type = CUDA_TRUE_FP16_AVAILABLE() ?
            CUDNN_DATA_HALF : CUDNN_DATA_FLOAT;
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                         this->dilation[0], this->dilation[1],
                                      CUDNN_CROSS_CORRELATION,
                                               compute_type));
#else
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                                                         1, 1,
                                    CUDNN_CROSS_CORRELATION));
#endif
#if CUDNN_VERSION_MIN(7, 0, 0)
        CUDNN_CHECK(cudnnSetConvolutionGroupCount(conv_desc, this->group));
        if (enable_tensor_core)
            CUDNN_CHECK(cudnnSetConvolutionMathType(conv_desc, CUDNN_TENSOR_OP_MATH));
#endif
        RunWithType<float16>();
#endif  // WITH_CUDA_FP16
    } else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CUDNN(Conv2d);

template <class Context> template <typename T>
void CuDNNConv2dGradientOp<Context>::ResetDesc() {
#if CUDNN_VERSION_MIN(5, 0, 0)
    CUDNN_CHECK(cudnnSetFilter4dDescriptor(filter_desc,
                                    CUDNNType<T>::type,
                                                format,
                        this->num_output / cudnn_group,
                          this->channels / this->group,
          this->kernel_size[0], this->kernel_size[1]));
#else
    CUDNN_CHECK(cudnnSetFilter4dDescriptor_v4(filter_desc,
                                       CUDNNType<T>::type,
                                                   format,
                           this->num_output / cudnn_group,
                             this->channels / this->group,
             this->kernel_size[0], this->kernel_size[1]));
#endif

    //  determine the input & output shape
    input_dims = Input(0).dims();
    cudnnSetTensor4dDescWithGroup<T>(&input_desc, this->data_format, Input(-1).dims(), cudnn_group);
    cudnnSetTensor4dDescWithGroup<T>(&output_desc, this->data_format, Input(0).dims(), cudnn_group);

    //  determine the bias shape
    if (HasBias()) {
        bias_offset = this->num_output / cudnn_group;
        if (this->data_format == "NCHW") {
            cudnnSetTensor4dDesc<T>(&bias_desc, this->data_format, vector<TIndex>({ 1, bias_offset, 1, 1 }));
        } else if (this->data_format == "NHWC") {
            cudnnSetTensor4dDesc<T>(&bias_desc, this->data_format, vector<TIndex>({ 1, 1, 1, bias_offset }));
        }
    }

    // determine the misc
    if (this->data_format == "NCHW") {
        this->x_offset = Input(0).count(1) / cudnn_group;
        this->y_offset = Input(-1).count(1) / cudnn_group;
    } else if (this->data_format == "NHWC") {
        this->x_offset = Input(0).dim(-1) / cudnn_group;
        this->y_offset = Input(-1).dim(-1) / cudnn_group;
    }

    CUDNN_CHECK(cudnnGetConvolutionBackwardFilterAlgorithm(handle[0],
                                                         output_desc,
                                                          input_desc,
                                                           conv_desc,
                                                         filter_desc,
                CUDNN_CONVOLUTION_BWD_FILTER_SPECIFY_WORKSPACE_LIMIT,
                                               WORKSPACE_LIMIT_BYTES,
                                                  &bwd_filter_algo));

    CUDNN_CHECK(cudnnGetConvolutionBackwardFilterWorkspaceSize(handle[0],
                                                             output_desc,
                                                              input_desc,
                                                               conv_desc,
                                                             filter_desc,
                                                         bwd_filter_algo,
                                            &workspace_bwd_filter_size));

    CUDNN_CHECK(cudnnGetConvolutionBackwardDataAlgorithm(handle[0],
                                                       filter_desc,
                                                        input_desc,
                                                         conv_desc,
                                                       output_desc,
                CUDNN_CONVOLUTION_BWD_DATA_SPECIFY_WORKSPACE_LIMIT,
                                             WORKSPACE_LIMIT_BYTES,
                                                  &bwd_data_algo));

    CUDNN_CHECK(cudnnGetConvolutionBackwardDataWorkspaceSize(handle[0],
                                                           filter_desc,
                                                            input_desc,
                                                             conv_desc,
                                                           output_desc,
                                                         bwd_data_algo,
                                            &workspace_bwd_data_size));
    if (workspace_bwd_data_size == 0) workspace_bwd_data_size += 1;
    if (workspace_bwd_filter_size == 0) workspace_bwd_filter_size += 1;
}

template <class Context> template <typename T>
void CuDNNConv2dGradientOp<Context>::RunWithType() {
    if (Input(0).dims() != input_dims) ResetDesc<T>();
    Tensor* buffer1 = ws()->GetBuffer();
    Tensor* buffer2 = ws()->GetBuffer();
    buffer1->Reshape(vector<TIndex>(1, cudnn_group * workspace_bwd_data_size));
    buffer2->Reshape(vector<TIndex>(1, cudnn_group * workspace_bwd_filter_size));

    const T* dYdata = Input(2).template data<T, Context>();
    for (int g = 0; g < cudnn_group; g++) {
        if (Output(2)->name() != "ignore") {
            T* dBdata = Output(2)->template mutable_data<T, Context>();
            CUDNN_CHECK(cudnnConvolutionBackwardBias(handle[g],
                            CUDNNType<T>::one, input_desc, dYdata + this->y_offset * g,
                              CUDNNType<T>::one, bias_desc, dBdata + bias_offset * g));
        }
        if (Output(1)->name() != "ignore") {
            auto* Xdata = Input(0).template data<T, Context>();
            auto* dWdata = Output(1)->template mutable_data<T, Context>();
            auto* workspace = buffer2->mutable_data<char, Context>();
            CUDNN_CHECK(cudnnConvolutionBackwardFilter(handle[1 * cudnn_group + g],
                            CUDNNType<T>::one, output_desc, Xdata + this->x_offset * g,
                                               input_desc, dYdata + this->y_offset * g,
                                                                             conv_desc,
                                                                       bwd_filter_algo,
                  workspace + g * workspace_bwd_filter_size, workspace_bwd_filter_size,
                    CUDNNType<T>::one, filter_desc, dWdata + this->weight_offset * g));
        }
        if (Output(0)->name() != "ignore") {
            auto* Wdata = Input(1).template data<T, Context>();
            auto* dXdata = Output(0)->template mutable_data<T, Context>();
            auto* workspace = buffer1->mutable_data<char, Context>();
            CUDNN_CHECK(cudnnConvolutionBackwardData(handle[2 * cudnn_group + g],
                            CUDNNType<T>::one, filter_desc, Wdata + this->weight_offset * g,
                                                    input_desc, dYdata + this->y_offset * g,
                                                                                  conv_desc,
                                                                              bwd_data_algo,
                           workspace + g * workspace_bwd_data_size, workspace_bwd_data_size,
                             CUDNNType<T>::zero, output_desc, dXdata + this->x_offset * g));
        }
    }
    kernel::Empty<T, Context>();
    ws()->ReleaseBuffer(buffer2);
    ws()->ReleaseBuffer(buffer1);
}

template <class Context>
void CuDNNConv2dGradientOp<Context>::RunOnDevice() {
#if CUDNN_VERSION_MAX(6, 0, 0)
    for (int i = 0; i < this->dilation.size(); i++)
        if (this->dilation[i] != 1) return Conv2dGradientOp<Context>::RunOnDevice();
#endif
    Conv2dGradientOp<Context>::GradientReshape();

    if (XIsType(Input(0), float)) {
#if CUDNN_VERSION_MIN(6, 0, 0)
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                         this->dilation[0], this->dilation[1],
                                      CUDNN_CROSS_CORRELATION,
                                           CUDNN_DATA_FLOAT));
#else
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                                                         1, 1,
                                    CUDNN_CROSS_CORRELATION));
#endif
#if CUDNN_VERSION_MIN(7, 0, 0)
        CUDNN_CHECK(cudnnSetConvolutionGroupCount(conv_desc, this->group));
        if (enable_tensor_core) 
            CUDNN_CHECK(cudnnSetConvolutionMathType(conv_desc, CUDNN_TENSOR_OP_MATH));
#endif
        RunWithType<float>();
    } else if (XIsType(Input(0), float16)) {
#ifdef WITH_CUDA_FP16
#if CUDNN_VERSION_MIN(6, 0, 0)
        //  may encounter CUDNN_STATUS_BAD_PARAM if using CUDNN_DATA_HALF
        //  keep it before cuDNN fix this bug
        //  compute_type = CUDA_TRUE_FP16_AVAILABLE() ?
        //      CUDNN_DATA_HALF : CUDNN_DATA_FLOAT;
        compute_type = CUDNN_DATA_FLOAT;
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                         this->dilation[0], this->dilation[1],
                                      CUDNN_CROSS_CORRELATION,
                                               compute_type));
#else
        CUDNN_CHECK(cudnnSetConvolution2dDescriptor(conv_desc,
                                   this->pad[0], this->pad[1],
                             this->stride[0], this->stride[1],
                              1, 1, CUDNN_CROSS_CORRELATION));
#endif
#if CUDNN_VERSION_MIN(7, 0, 0)
        CUDNN_CHECK(cudnnSetConvolutionGroupCount(conv_desc, this->group));
        if (enable_tensor_core)
            CUDNN_CHECK(cudnnSetConvolutionMathType(conv_desc, CUDNN_TENSOR_OP_MATH));
#endif
        RunWithType<float16>();
#endif  // WITH_CUDA_FP16
    } else LOG(FATAL) << DTypeHelper(Input(0), { "float32", "float16" });
}

DEPLOY_CUDNN(Conv2dGradient);

}    // namespace dragon

#endif    // WITH_CUDNN