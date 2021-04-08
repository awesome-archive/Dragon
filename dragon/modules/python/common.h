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

#ifndef DRAGON_MODULES_PYTHON_COMMON_H_
#define DRAGON_MODULES_PYTHON_COMMON_H_

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include "dragon/core/common.h"
#include "dragon/core/context.h"
#include "dragon/core/context_cuda.h"
#include "dragon/core/operator.h"
#include "dragon/core/registry.h"
#include "dragon/core/workspace.h"
#include "dragon/modules/python/types.h"
#include "dragon/onnx/onnx_backend.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace dragon {

namespace python {

namespace py = pybind11;

} // namespace python

} // namespace dragon

#endif // DRAGON_MODULES_PYTHON_COMMON_H_
