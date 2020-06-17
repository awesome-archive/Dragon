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

"""The basic idea of directly run operators comes from ``caffe2``,
it spends much more time on Python frontend than C++ backend,
which should not be taken for running computation-intensive operators.

We extend a new ``PERSISTENT`` engine, that hashes the arguments
as many as possible, i.e., creates a operator once while running
with arbitrary inputs and outputs many times.

Note that it is still a challenge to persist the operators which
take the argument with uncertain numerical bounds. In this case,
our engine will still create lots of duplicates.

"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from dragon.core.framework import context
from dragon.core.framework import workspace
from dragon.core.util import nest
from dragon.core.util import six
from dragon.vm.torch.autograd import grad_mode
from dragon.vm.torch.cpp import device as Device
from dragon.vm.torch.jit import tape
from dragon.vm.torch.tensor import Tensor


def run_operator(
    op_def,
    inputs,
    outputs,
    no_grad=False,
    pre_callback=None,
):
    inputs = nest.flatten(inputs)
    outputs = nest.flatten(outputs)

    if len(outputs) == 0:
        raise ValueError(
            'The number of <outputs> should be '
            'at least 1. Got {}.'.format(len(outputs))
        )

    requires_grad = False
    input_names, output_names = [], []

    for input in inputs:
        input_names.append(input.id)
        if input.requires_grad:
            requires_grad = True

    requires_grad = requires_grad and grad_mode.is_grad_enabled()

    # Allocate outputs.
    ws = workspace.get_workspace()
    output_scope = context.get_eager_scope(requires_grad)
    gc = ws.collectors  # Garbage collectors

    for i, spec in enumerate(outputs):
        if isinstance(spec, six.string_types):
            output_names.append(spec)
        else:
            if isinstance(spec, Device):
                output_id = gc.TENSOR.alloc(output_scope)
                ref = Tensor(device=spec)
                ref.__gc__, ref._id = gc.TENSOR, output_id
                ref._impl = ws.CreateTensor(output_id)
                outputs[i] = ref
            output_names.append(outputs[i].id)

    # Generate the OpDef.
    default_tape = tape.get_default_tape()
    op_def = op_def.DeriveTo(input_names, output_names)

    # Maybe record this operation for future developments.
    if default_tape is not None:
        default_tape.add_def(op_def)
        requires_grad = requires_grad or default_tape.retain_graph

    if len(inputs) > 0 and no_grad is False:
        if requires_grad:
            ignores = set()
            instance_tape = tape.Tape()
            for input in inputs:
                instance_tape.merge_from(input.__tape__)
                ignores = ignores.union(input._ignored_grads)
            op_def.name = gc.OPERATOR.alloc(op_def.type)
            instance_tape.add_operation(op_def)
            for output in outputs:
                output.requires_grad = True
                output._ignored_grads = ignores
                output.__tape__ = instance_tape
        else:
            if default_tape is not None and default_tape.retain_ops:
                op_def.name = gc.OPERATOR.alloc(op_def.type)
            for output in outputs:
                output.requires_grad = False

    # Dispatch the computation.
    if pre_callback is not None:
        pre_callback(op_def.name)
    workspace.run_operator(op_def)

    # Return the outputs.
    return outputs if len(outputs) > 1 else outputs[0]
