# ------------------------------------------------------------
# Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
#
# Licensed under the BSD 2-Clause License.
# You should have received a copy of the BSD 2-Clause License
# along with the software. If not, See,
#
#    <https://opensource.org/licenses/BSD-2-Clause>
#
# ------------------------------------------------------------

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from dragon.core.framework import ops
from dragon.core.framework.ops import Operator


class Initializer(Operator):
    def __init__(self, key, dev, **kwargs):
        super(Initializer, self).__init__(key, dev, **kwargs)
        self.ndim = kwargs.get('ndim', 0)
        self.dtype = kwargs.get('dtype', 'float32')

    def feed(self, handle, shape):
        for i, e in enumerate(shape):
            self.feed_arg(
                '{}/dims[{}]'
                .format(handle, i),
                e, 'int64'
            )

    def forward(
        self,
        shape,
        out=None,
        shape_like=None,
        trainable=False,
    ):
        inputs = [] if shape_like is None else [shape_like]
        outputs = [
            ops.new_leaf(
                shape=shape,
                dtype=self.dtype,
                device=self.alloc(),
                trainable=trainable,
            ) if out is None else out
        ]
        return self.dispatch(
            inputs, outputs,
            callback=lambda handle:
            self.feed(handle, shape)
        )


class Eye(Initializer):
    def __init__(self, key, dev, **kwargs):
        super(Eye, self).__init__(key, dev, **kwargs)
        self.k = kwargs.get('k', 0)

    def attributes(self):
        return {
            'op_type': 'Eye',
            'arguments': {
                'k': self.k,
                'dtype': self.dtype,
                'dims_descs': [
                    '${{HANDLE}}/dims[{}]'.format(n)
                    for n in range(self.ndim)
                ],
            },
        }


class Fill(Initializer):
    def __init__(self, key, dev, **kwargs):
        super(Fill, self).__init__(key, dev, **kwargs)
        self.value = kwargs.get('value', 0.)

    def attributes(self):
        return {
            'op_type': 'Fill',
            'arguments': {
                'dtype': self.dtype,
                'value': float(self.value),
                'dims_descs': [
                    '${{HANDLE}}/dims[{}]'.format(n)
                    for n in range(self.ndim)
                ],
            },
        }


class GlorotNormal(Initializer):
    def __init__(self, key, dev, **kwargs):
        super(GlorotNormal, self).__init__(key, dev, **kwargs)
        self.scale = kwargs.get('scale', 2.)
        self.mode = kwargs.get('mode', 'FAN_IN')

    def attributes(self):
        return {
            'op_type': 'GlorotNormal',
            'arguments': {
                'dtype': self.dtype,
                'scale': float(self.scale),
                'mode': self.mode.lower(),
                'dims_descs': [
                    '${{HANDLE}}/dims[{}]'.format(n)
                    for n in range(self.ndim)
                ],
            },
        }


class GlorotUniform(Initializer):
    def __init__(self, key, dev, **kwargs):
        super(GlorotUniform, self).__init__(key, dev, **kwargs)
        self.scale = kwargs.get('scale', 3.)
        self.mode = kwargs.get('mode', 'FAN_IN')

    def attributes(self):
        return {
            'op_type': 'GlorotUniform',
            'arguments': {
                'dtype': self.dtype,
                'scale': float(self.scale),
                'mode': self.mode.lower(),
                'dims_descs': [
                    '${{HANDLE}}/dims[{}]'.format(n)
                    for n in range(self.ndim)
                ],
            },
        }


class RandomNormal(Initializer):
    def __init__(self, key, dev, **kwargs):
        super(RandomNormal, self).__init__(key, dev, **kwargs)
        self.mean = kwargs.get('mean', 0.)
        self.std = kwargs.get('std', 1.)

    def attributes(self):
        return {
            'op_type': 'RandomNormal',
            'arguments': {
                'dtype': self.dtype,
                'mean': float(self.mean),
                'std': float(self.std),
                'dims_descs': [
                    '${{HANDLE}}/dims[{}]'.format(n)
                    for n in range(self.ndim)
                ],
            },
        }


class RandomUniform(Initializer):
    def __init__(self, key, dev, **kwargs):
        super(RandomUniform, self).__init__(key, dev, **kwargs)
        self.low = kwargs.get('low', 0.)
        self.high = kwargs.get('high', 1.)

    def attributes(self):
        return {
            'op_type': 'RandomUniform',
            'arguments': {
                'dtype': self.dtype,
                'low': float(self.low),
                'high': float(self.high),
                'dims_descs': [
                    '${{HANDLE}}/dims[{}]'.format(n)
                    for n in range(self.ndim)
                ],
            },
        }


class TruncatedNormal(Initializer):
    def __init__(self, key, dev, **kwargs):
        super(TruncatedNormal, self).__init__(key, dev, **kwargs)
        self.mean = kwargs.get('mean', 0.)
        self.std = kwargs.get('std', 1.)

    def attributes(self):
        return {
            'op_type': 'TruncatedNormal',
            'arguments': {
                'dtype': self.dtype,
                'mean': float(self.mean),
                'std': float(self.std),
                'dims_descs': [
                    '${{HANDLE}}/dims[{}]'.format(n)
                    for n in range(self.ndim)
                ],
            },
        }
