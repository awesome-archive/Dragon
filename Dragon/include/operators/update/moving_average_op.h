// ------------------------------------------------------------
// Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
//
// Licensed under the BSD 2-Clause License.
// You should have received a copy of the BSD 2-Clause License
// along with the software. If not, See,
//
//      <https://opensource.org/licenses/BSD-2-Clause>
//
// -------------------------------------------------------------

#ifndef DRAGON_OPERATORS_UPDATE_MOVING_AVERAGE_OP_H_
#define DRAGON_OPERATORS_UPDATE_MOVING_AVERAGE_OP_H_

#include "core/operator.h"

namespace dragon {

template <class Context>
class MovingAverageOp final : public Operator<Context> {
 public:
    MovingAverageOp(const OperatorDef& op_def, Workspace* ws)
        : Operator<Context>(op_def, ws), 
          decay(OperatorBase::GetSingleArg<float>("decay", 1.0)) {}
    USE_OPERATOR_FUNCTIONS(Context);

    void RunOnDevice() override;
    template <typename T> void RunWithType();

  protected:
    float decay;

};

}   // namespace dragon


#endif  // DRAGON_OPERATORS_UPDATE_MOVING_AVERAGE_OP_H_