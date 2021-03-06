# ------------------------------------------------------------
# Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
#
# Licensed under the BSD 2-Clause License.
# You should have received a copy of the BSD 2-Clause License
# along with the software. If not, See,
#
#      <https://opensource.org/licenses/BSD-2-Clause>
#
# ------------------------------------------------------------

"""Some useful mappings are defined here."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy


TENSOR_TYPE_TO_NP_TYPE = {
    'bool': numpy.bool,
    'int8': numpy.int8,
    'uint8': numpy.uint8,
    'int32': numpy.int32,
    'int64': numpy.int64,
    'float16': numpy.float16,
    'float32': numpy.float32,
    'float64': numpy.float64,
}


TENSOR_TYPE_TO_TORCH_TENSOR = {
    'int8': 'CharTensor',
    'uint8': 'ByteTensor',
    'int32': 'IntTensor',
    'int64': 'LongTensor',
    'float16': 'HalfTensor',
    'float32': 'FloatTensor',
    'float64': 'DoubleTensor',
}