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

#ifndef DRAGON_OPERATORS_ACTIVATION_PRELU_OP_H_
#define DRAGON_OPERATORS_ACTIVATION_PRELU_OP_H_

#include "core/operator.h"

namespace dragon {

template <class Context>
class PReluOp final : public Operator<Context> {
 public:
    PReluOp(const OperatorDef& def, Workspace* ws)
        : Operator<Context>(def, ws),
          channel_shared_(OpArg<bool>(
              "channel_shared", false)) {}
    USE_OPERATOR_FUNCTIONS;

    void RunOnDevice() override;
    template <typename T> void RunImpl();

 protected:
    int64_t channel_shared_, channels_, dim_;
};

template <class Context>
class PReluGradientOp final : public Operator<Context> {
 public:
    PReluGradientOp(const OperatorDef& def, Workspace* ws)
        : Operator<Context>(def, ws),
          channel_shared_(OpArg<bool>(
              "channel_shared", false)) {}
    USE_OPERATOR_FUNCTIONS;

    void RunOnDevice() override;
    template <typename T> void RunImpl();

 protected:
    int64_t channel_shared_, channels_, dim_;
};

}  // namespace dragon

#endif  // DRAGON_OPERATORS_ACTIVATION_PRELU_OP_H_