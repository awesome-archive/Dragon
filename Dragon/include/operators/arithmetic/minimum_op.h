/*!
 * Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
 *
 * Licensed under the BSD 2-Clause License.
 * You should have received a copy of the BSD 2-Clause License
 * along with the software. If not, See,
 *
 *      <https://opensource.org/licenses/BSD-2-Clause>
 *
 * ------------------------------------------------------------
 */

#ifndef DRAGON_OPERATORS_ARITHMETIC_MINIMUM_OP_H_
#define DRAGON_OPERATORS_ARITHMETIC_MINIMUM_OP_H_

#include "core/operator.h"

namespace dragon {

template <class Context>
class MinimumOp final : public Operator<Context> {
 public:
    USE_SIMPLE_CTOR_DTOR(MinimumOp);
    USE_OPERATOR_FUNCTIONS;

    void RunOnDevice() override;
    template <typename T> void RunWithType();
    template <typename T> void EltwiseRunWithType();
    template <typename T> void BroadcastRunWithType();
};

template <class Context>
class MinimumGradientOp final : public Operator<Context> {
 public:
    USE_SIMPLE_CTOR_DTOR(MinimumGradientOp);
    USE_OPERATOR_FUNCTIONS;

    void RunOnDevice() override;
    template <typename T> void RunWithType();
    template <typename T> void EltwiseRunWithType();
    template <typename T> void BroadcastRunWithType();
};

}  // namespace dragon

#endif  // DRAGON_OPERATORS_ARITHMETIC_MINIMUM_OP_H_