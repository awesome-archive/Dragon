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

from dragon.vm.torch.autograd import no_grad
from dragon.vm.torch.tensor import _ReferenceTensor
from dragon.vm.torch.ops.modules.base import BaseModule


class Indexing(BaseModule):
    """This module imports the *CropOp* from backend.

    Arbitrary length of starts and sizes will be take,
    and the resulting memory is deep copied.

    """
    def __init__(self, key, dev, **kwargs):
        super(Indexing, self).__init__(key, dev, **kwargs)
        self.n_starts = kwargs.get('n_starts', 0)
        self.n_sizes = kwargs.get('n_sizes', 0)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Crop',
            'arguments': {
                'starts_desc': [
                    '${{ANCHOR}}/starts[{}]'.format(n)
                        for n in range(self.n_starts)],
                'sizes_desc': [
                    '${{ANCHOR}}/sizes[{}]'.format(n)
                        for n in range(self.n_sizes)],
            },
        }

    def update_arguments(self, A, starts, sizes):
        for i, e in enumerate(starts):
            self.set_argument_i64('{}/starts[{}]'.format(A, i), e)
            self.set_argument_i64('{}/sizes[{}]'.format(A, i), sizes[i])

    def forward(self, x, starts, sizes):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [self.register_output()]
        callback = lambda A: self.update_arguments(A, starts, sizes)
        return self.run(inputs, outputs, callback=callback)


class Assigning(BaseModule):
    """This module imports the *AssignOp* from backend.

    Arbitrary length of starts and sizes will be take.

    """
    def __init__(self, key, dev, **kwargs):
        super(Assigning, self).__init__(key, dev, **kwargs)
        self.n_starts = kwargs.get('n_starts', 0)
        self.n_sizes = kwargs.get('n_sizes', 0)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Assign',
            'arguments': {
                'starts_desc': [
                    '${{ANCHOR}}/starts[{}]'.format(n)
                        for n in range(self.n_starts)],
                'sizes_desc': [
                    '${{ANCHOR}}/sizes[{}]'.format(n)
                        for n in range(self.n_sizes)],
            },
        }

    def update_arguments(self, A, starts, sizes):
        for i, e in enumerate(starts):
            self.set_argument_i64('{}/starts[{}]'.format(A, i), e)
            self.set_argument_i64('{}/sizes[{}]'.format(A, i), sizes[i])

    def forward(self, x, y, starts, sizes):
        self.unify_devices([x, y])
        callback = lambda A: self.update_arguments(A, starts, sizes)
        return self.run([x], [y], callback=callback, auto_grad=False)


class Concat(BaseModule):
    """This module imports the *ConcatOp* from backend.

    Concatenate the inputs along the given axis.

    """
    def __init__(self, key, dev, **kwargs):
        super(Concat, self).__init__(key, dev, **kwargs)
        self.axis = kwargs.get('axis', 0)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Concat',
            'arguments': {
                'axis': self.axis
            },
        }

    def forward(self, xs, y):
        inputs = xs; self.unify_devices(inputs)
        outputs = [y] if y else [self.register_output()]
        return self.run(inputs, outputs)


class Gather(BaseModule):
    """This module imports the *GatherOp* from backend.

    Gather the input according to the indices along the given axis,
    and the resulting shape should be:

        input.shape[:axis] + indices.shape + input.shape[axis + 1:]

    """
    def __init__(self, key, dev, **kwargs):
        super(Gather, self).__init__(key, dev, **kwargs)
        self.axis = kwargs.get('axis', 0)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Gather',
            'arguments': {
                'axis': self.axis,
            },
        }

    def forward(self, x, indices, y):
        inputs = [x, indices]; self.unify_devices(inputs)
        outputs = [y] if y else [self.register_output()]
        return self.run(inputs, outputs)


class Reduce(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(Reduce, self).__init__(key, dev, **kwargs)
        self.operation = kwargs.get('operation', 'SUM')
        self.dim = kwargs.get('dim', None)
        self.keepdim = kwargs.get('keepdim', True)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Reduce',
            'arguments': {
                'operation': self.operation,
                'axes': [self.dim] if self.dim is not None else None,
                'keep_dims': self.keepdim,
            },
        }

    def forward(self, x, y):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [y] if y else [self.register_output()]
        return self.run(inputs, outputs)


class ArgReduce(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(ArgReduce, self).__init__(key, dev, **kwargs)
        self.operation = kwargs.get('operation', 'ARGMAX')
        self.axis = kwargs.get('axis', None)
        self.keepdim = kwargs.get('keepdim', True)
        self.top_k = kwargs.get('top_k', 1)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'ArgReduce',
            'arguments': {
                'operation': self.operation if 'ARG' in self.operation \
                    else 'ARG' + self.operation,
                'axis': self.axis if self.axis else 2147483647,
                'keep_dims': self.keepdim,
                'top_k': self.top_k,
            },
        }

    def forward(self, x, y):
        inputs = [x]; self.unify_devices(inputs)
        if 'ARG' in self.operation:
            # Return indices only
            outputs = [y] if y else [self.register_output()]
            outputs += [self.register_output()]
            returns = self.run(inputs, outputs)
            return returns[0]
        else:
            if y:
                if not isinstance(y, (tuple, list)):
                    raise TypeError('Excepted outputs as a tuple or list, got {}.'.format(type(y)))
                if len(y) != 2:
                    raise ValueError('Excepted 2 outputs, got {}.'.format(len(y)))
                outputs = [y[1], y[0]]
            else: outputs = [self.register_output(), self.register_output()]
            returns = self.run(inputs, outputs)
            # Return values only
            if self.axis is None: return returns[1]
            # Return values and indices
            return returns[1], returns[0]


class Reshape(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(Reshape, self).__init__(key, dev, **kwargs)
        self.n_dim = kwargs.get('n_dim', 0)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Reshape',
            'arguments': {
                'dims_desc': [
                    '${{ANCHOR}}/dims[{}]'.format(n)
                        for n in range(self.n_dim)
                ],
            },
        }

    def update_arguments(self, A, shape):
        for i, e in enumerate(shape):
            self.set_argument_i64('{}/dims[{}]'.format(A, i), e)

    def forward(self, x, shape):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [_ReferenceTensor(x)]
        callback = lambda A: self.update_arguments(A, shape)
        return self.run(inputs, outputs, callback=callback)


class Squeeze(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(Squeeze, self).__init__(key, dev, **kwargs)
        self.dim = kwargs.get('dim', None)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Squeeze',
            'arguments': {'axis': self.dim},
        }

    def forward(self, x, out=None):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [out] if out else [_ReferenceTensor(x)]
        return self.run(inputs, outputs)


class UnSqueeze(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(UnSqueeze, self).__init__(key, dev, **kwargs)
        self.dim = kwargs.get('dim', None)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'ExpandDims',
            'arguments': {'axis': self.dim},
        }

    def forward(self, x, out=None):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [out] if out else [_ReferenceTensor(x)]
        return self.run(inputs, outputs)


class Permute(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(Permute, self).__init__(key, dev, **kwargs)
        self.n_perm = kwargs.get('n_perm', 0)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Transpose',
            'arguments': {
                'perm_desc': ['${{ANCHOR}}/perm[{}]'.format(n)
                    for n in range(self.n_perm)],
            },
        }

    def update_arguments(self, A, perm):
        if perm:
            for i, e in enumerate(perm):
                self.set_argument_i64('{}/perm[{}]'.format(A, i), e)

    def forward(self, x, perm):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [self.register_output()]
        callback = lambda A: self.update_arguments(A, perm)
        return self.run(inputs, outputs, callback=callback)


class Repeat(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(Repeat, self).__init__(key, dev, **kwargs)
        self.n_times = kwargs.get('n_times', 0)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Tile',
            'arguments': {
                'multiples_desc': [
                    '${{ANCHOR}}/multiples[{}]'.format(n)
                        for n in range(self.n_times)
                ],
            },
        }

    def update_arguments(self, A, times):
        for i, d in enumerate(times):
            self.set_argument_i64('{}/multiples[{}]'.format(A, i), d)

    def forward(self, x, times):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [self.register_output()]
        callback = lambda A: self.update_arguments(A, times)
        return self.run(inputs, outputs, callback=callback)


class OneHot(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(OneHot, self).__init__(key, dev, **kwargs)
        self.depth = kwargs.get('depth', 1)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'OneHot',
            'arguments': {
                'depth': self.depth,
            },
        }

    def forward(self, x):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [self.register_output()]
        with no_grad(): return self.run(inputs, outputs)


class Cast(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(Cast, self).__init__(key, dev, **kwargs)
        self.dtype = kwargs.get('dtype', 'float32')
        self.inplace = kwargs.get('inplace', False)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Cast',
            'arguments': {
                'dtype': self.dtype,
                'inplace': self.inplace,
            },
        }

    def forward(self, x):
        if x.dtype == self.dtype: return x
        if not self.inplace:
            inputs = [x]; self.unify_devices(inputs)
            outputs = [self.register_output()]
            y = self.run(inputs, outputs)
        else:
            self.unify_devices([x])
            with no_grad(): y = self.run([], [x])
        return y


class Multinomial(BaseModule):
    def __init__(self, key, dev, **kwargs):
        super(Multinomial, self).__init__(key, dev, **kwargs)
        self.num_samples = kwargs.get('num_samples', 1)
        self.normalize = kwargs.get('normalize', False)
        self.register_op()

    def register_op(self):
        self.op_meta = {
            'op_type': 'Multinomial',
            'arguments': {
                'num_samples': self.num_samples,
                'normalize': self.normalize,
            },
        }

    def forward(self, x, y):
        inputs = [x]; self.unify_devices(inputs)
        outputs = [y] if y else [self.register_output()]
        with no_grad(): return self.run(inputs, outputs)