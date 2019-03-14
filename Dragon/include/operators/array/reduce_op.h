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

#ifndef DRAGON_OPERATORS_ARRAY_REDUCE_OP_H_
#define DRAGON_OPERATORS_ARRAY_REDUCE_OP_H_

#include "core/operator.h"

namespace dragon {

template <class Context>
class ReduceOp final : public Operator<Context> {
 public:
    ReduceOp(const OperatorDef& def, Workspace* ws)
        : Operator<Context>(def, ws),
          axes(OperatorBase::Args<int64_t>("axes")),
          keep_dims(OperatorBase::Arg<bool>("keep_dims", false)),
          operation(OperatorBase::Arg<string>("operation", "SUM")) {}
    USE_OPERATOR_FUNCTIONS;

    void RunOnDevice() override;
    template <typename T> void RunWithType();

 protected:
    string operation;
    int64_t keep_dims;
    vector<int64_t> dims, axes;
    vector<int> dims32, axes32;
};

template <class Context>
class ReduceGradientOp final : public Operator<Context> {
 public:
    ReduceGradientOp(const OperatorDef& def, Workspace* ws)
        : Operator<Context>(def, ws),
          axes(OperatorBase::Args<int64_t>("axes")),
          operation(OperatorBase::Arg<string>("operation", "SUM")) {}
    USE_OPERATOR_FUNCTIONS;

    void RunOnDevice() override;
    template <typename T> void RunWithType();

 protected:
    string operation;
    int64_t axis, outer_dim, inner_dim, axis_dim;
    vector<int64_t> axes, y_dimsV, y_stridesV;
    vector<int> dims32, axes32;
    Tensor x_dimsT, y_dimsT, y_stridesT;
};

}  // namespace dragon

#endif  // DRAGON_OPERATORS_ARRAY_REDUCE_OP_H_