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

#ifndef DRAGON_OPERATORS_ARRAY_NON_ZERO_OP_H_
#define DRAGON_OPERATORS_ARRAY_NON_ZERO_OP_H_

#include "core/operator.h"

namespace dragon {

template <class Context>
class NonZeroOp final : public Operator<Context> {
 public:
    NonZeroOp(const OperatorDef& def, Workspace* ws)
        : Operator<Context>(def, ws) {}
    USE_OPERATOR_FUNCTIONS;

    void RunOnDevice() override;
    template <typename T> void RunImpl();

 protected:
    Tensor X_dims_;
};

}  // namespace dragon

#endif  // DRAGON_OPERATORS_ARRAY_NON_ZERO_OP_H_