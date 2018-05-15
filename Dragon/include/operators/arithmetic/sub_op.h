// ------------------------------------------------------------
// Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
//
// Licensed under the BSD 2-Clause License.
// You should have received a copy of the BSD 2-Clause License
// along with the software. If not, See,
//
//      <https://opensource.org/licenses/BSD-2-Clause>
//
// ------------------------------------------------------------

#ifndef DRAGON_OPERATORS_ARITHMETIC_SUB_OP_H_
#define DRAGON_OPERATORS_ARITHMETIC_SUB_OP_H_

#include "core/operator.h"

namespace dragon {

template <class Context>
class SubOp final : public Operator<Context> {
 public:
    USE_SIMPLE_CTOR_DTOR(SubOp);
    USE_OPERATOR_FUNCTIONS(Context);

    void RunOnDevice() override;
    template <typename T> void EltwiseRunWithType();
    template <typename T> void BroadcastRunWithType(int type);

 protected:
    Tensor* bcast_multiplier;
};

template <class Context>
class SubGradientOp final : public Operator<Context> {
 public:
    USE_SIMPLE_CTOR_DTOR(SubGradientOp);
    USE_OPERATOR_FUNCTIONS(Context);

    void RunOnDevice() override;
    template <typename T> void EltwiseRunWithType();
    template <typename T> void BroadcastRunWithType(int type);

 protected:
    Tensor* bcast_multiplier;
};

template <class Context>
class RSubOp final : public Operator<Context> {
 public:
    USE_SIMPLE_CTOR_DTOR(RSubOp);
    USE_OPERATOR_FUNCTIONS(Context);

    void RunOnDevice() override;
    template <typename T> void EltwiseRunWithType();
    template <typename T> void BroadcastRunWithType(int type);

 protected:
    Tensor* bcast_multiplier;
};

template <class Context>
class RSubGradientOp final : public Operator<Context> {
 public:
    USE_SIMPLE_CTOR_DTOR(RSubGradientOp);
    USE_OPERATOR_FUNCTIONS(Context);

    void RunOnDevice() override;
    template <typename T> void EltwiseRunWithType();
    template <typename T> void BroadcastRunWithType(int type);

 protected:
    Tensor* bcast_multiplier;
};

}    // namespace dragon

#endif    // DRAGON_OPERATORS_ARITHMETIC_SUB_OP_H_