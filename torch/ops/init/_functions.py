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

from dragon.vm.torch.autograd import function


class _Initializer(function.Function):
    def __init__(self, key, dev, **kwargs):
        super(_Initializer, self).__init__(key, dev, **kwargs)
        self.ndim = kwargs.get('ndim', 0)
        self.dtype = kwargs.get('dtype', 'float32')

    def feed(self, handle, shape):
        for i in range(self.ndim):
            self.feed_arg(
                '{}/dims[{}]'
                .format(handle, i),
                shape[i], 'int64',
            )

    def forward(self, out, shape, shape_like=None):
        return self.dispatch(
            [] if shape_like is None else [shape_like], [out],
            callback=lambda handle:
                self.feed(handle, shape),
        )


class Arange(function.Function):
    def __init__(self, key, dev, **kwargs):
        super(Arange, self).__init__(key, dev, **kwargs)
        self.num_args = kwargs.get('num_args', 3)
        self.dtype = kwargs.get('dtype', 'int64')

    def attributes(self):
        return {
            'op_type': 'Arange',
            'arguments': {
                'dtype': self.dtype,
                'slice_descs': [
                    '${{HANDLE}}/slice[{}]'
                    .format(n) for n in range(self.num_args)
                ],
            }
        }

    def feed(self, handle, slice_args):
        for i in range(len(slice_args)):
            self.feed_arg(
                '{}/slice[{}]'
                .format(handle, i),
                slice_args[i], 'float32'
            )

    def forward(self, slice_args):
        return self.dispatch(
            [], [self.alloc()],
            callback=lambda handle:
            self.feed(handle, slice_args)
        )


class Eye(_Initializer):
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


class Fill(_Initializer):
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


class RandomNormal(_Initializer):
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


class RandomUniform(_Initializer):
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
