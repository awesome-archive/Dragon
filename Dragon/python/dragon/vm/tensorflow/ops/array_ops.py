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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import dragon

from dragon.vm.tensorflow.framework import dtypes


def expand_dims(input, axis=None, name=None, dim=None):
    if dim is not None:
        if axis is not None:
            raise ValueError("cannot specify both 'axis' and 'dim'.")
        axis = dim
    return dragon.ops.ExpandDims(input, axis=axis, name=name)


def shape(input, name=None, out_type=dtypes.float32):
    return dragon.ops.Shape(input, name=name)


def zeros(shape, dtype=dtypes.float32, name=None):
    return dragon.ops.Fill(shape, value=0.0, dtype=dtype.name, name=name)


def ones(shape, dtype=dtypes.float32, name=None):
    return dragon.ops.Fill(shape, value=1.0, dtype=dtype.name, name=name)


def placeholder(dtype, shape=None, name=None):
    # Check data type
    if dtype is not None:
        if not isinstance(dtype, dtypes.DType):
            raise TypeError('The dtype should be a valid tensorflow data type.')

    # Construct a tensor from the explicit name
    return dragon.Tensor.Ref(
        dragon.workspace.GetDummyName(
            dragon.get_default_name_scope() + name
                if name else 'Placeholder',
                    suffix=':0', domain='Tensor'),
        dtype=dtype.name, shape=shape).Placeholder()


def concat(values, axis, name=None):
    return dragon.ops.Concat(values, axis=axis, name=name)


def transpose(a, perm=None, name=None):
    return dragon.ops.Transpose(a, perm=perm, name=name)


def tile(input, multiples, name=None):
    return dragon.ops.Tile(input, multiples=multiples, name=name)


def pad(tensor, paddings, mode="CONSTANT", name=None, constant_values=0):
    return dragon.ops.Pad(tensor, paddings, mode=mode, name=name, value=constant_values)


def reshape(tensor, shape, name=None):
    return dragon.ops.Reshape(tensor, shape=shape, name=name)
