/*!
 * Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
 *
 * Licensed under the BSD 2-Clause License.
 * You should have received a copy of the BSD 2-Clause License
 * along with the software. If not, See,
 *
 *     <https://opensource.org/licenses/BSD-2-Clause>
 *
 * ------------------------------------------------------------
 */

#ifndef DRAGON_OPERATORS_ACTIVATION_TANH_OP_H_
#define DRAGON_OPERATORS_ACTIVATION_TANH_OP_H_

#include "dragon/core/operator.h"

namespace dragon {

template <class Context>
class TanhOp final : public Operator<Context> {
 public:
  SIMPLE_CTOR_DTOR(TanhOp);
  USE_OPERATOR_FUNCTIONS;

  void RunOnDevice() override {
    DispatchHelper<dtypes::Floating>::Call(this, Input(0));
  }

  template <typename T>
  void DoRunWithType();
};

template <class Context>
class TanhGradientOp final : public Operator<Context> {
 public:
  SIMPLE_CTOR_DTOR(TanhGradientOp);
  USE_OPERATOR_FUNCTIONS;

  void RunOnDevice() override {
    DispatchHelper<dtypes::Floating>::Call(this, Input(0));
  }

  template <typename T>
  void DoRunWithType();
};

#ifdef USE_CUDNN

template <class Context>
class CuDNNTanhOp final : public Operator<Context> {
 public:
  CuDNNTanhOp(const OperatorDef& def, Workspace* ws)
      : Operator<Context>(def, ws) {
    CuDNNCreateTensorDesc(&input_desc_);
    CUDNN_CHECK(cudnnCreateActivationDescriptor(&act_desc_));
    CUDNN_CHECK(cudnnSetActivationDescriptor(
        act_desc_, CUDNN_ACTIVATION_TANH, CUDNN_PROPAGATE_NAN, 0));
  }
  USE_OPERATOR_FUNCTIONS;

  ~CuDNNTanhOp() {
    CuDNNDestroyTensorDesc(&input_desc_);
    CUDNN_CHECK(cudnnDestroyActivationDescriptor(act_desc_));
  }

  void RunOnDevice() override {
    DispatchHelper<dtypes::Floating>::Call(this, Input(0));
  }

  template <typename T>
  void DoRunWithType();

 protected:
  cudnnTensorDescriptor_t input_desc_;
  cudnnActivationDescriptor_t act_desc_;
};

template <class Context>
class CuDNNTanhGradientOp final : public Operator<Context> {
 public:
  CuDNNTanhGradientOp(const OperatorDef& def, Workspace* ws)
      : Operator<Context>(def, ws) {
    CuDNNCreateTensorDesc(&input_desc_);
    CUDNN_CHECK(cudnnCreateActivationDescriptor(&act_desc_));
    CUDNN_CHECK(cudnnSetActivationDescriptor(
        act_desc_, CUDNN_ACTIVATION_TANH, CUDNN_PROPAGATE_NAN, 0));
  }
  USE_OPERATOR_FUNCTIONS;

  ~CuDNNTanhGradientOp() {
    CuDNNDestroyTensorDesc(&input_desc_);
    CUDNN_CHECK(cudnnDestroyActivationDescriptor(act_desc_));
  }

  void RunOnDevice() override {
    DispatchHelper<dtypes::Floating>::Call(this, Input(0));
  }

  template <typename T>
  void DoRunWithType();

 protected:
  cudnnTensorDescriptor_t input_desc_;
  cudnnActivationDescriptor_t act_desc_;
};

#endif // USE_CUDNN

} // namespace dragon

#endif // DRAGON_OPERATORS_ACTIVATION_TANH_OP_H_
