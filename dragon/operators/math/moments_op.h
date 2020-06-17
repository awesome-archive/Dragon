/*!
 * Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
 *
 * Licensed under the BSD 2-Clause License.
 * You should have received a copy of the BSD 2-Clause License
 * along with the software. If not, See,
 *
 *    <https://opensource.org/licenses/BSD-2-Clause>
 *
 * ------------------------------------------------------------
 */

#ifndef DRAGON_OPERATORS_MATH_MOMENTS_OP_H_
#define DRAGON_OPERATORS_MATH_MOMENTS_OP_H_

#include "dragon/core/operator.h"

namespace dragon {

template <class Context>
class MomentsOp final : public Operator<Context> {
 public:
  MomentsOp(const OperatorDef& def, Workspace* ws)
      : Operator<Context>(def, ws),
        axes_(OpArgs<int64_t>("axes")),
        keep_dims_(OpArg<int64_t>("keep_dims", 0)) {}
  USE_OPERATOR_FUNCTIONS;

  void RunOnDevice() override;

  template <typename Tx, typename Ty>
  void DoRunWithType();

 protected:
  int64_t keep_dims_;
  vec64_t axes_;
};

} // namespace dragon

#endif // DRAGON_OPERATORS_MATH_MOMENTS_OP_H_
