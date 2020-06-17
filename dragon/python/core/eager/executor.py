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

from dragon.core.eager import backprop
from dragon.core.eager.tensor import EagerTensor
from dragon.core.framework import device_spec
from dragon.core.framework import context
from dragon.core.framework import workspace
from dragon.core.util import nest
from dragon.core.util import six


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
    tape = backprop.get_default_tape()

    for x in inputs:
        input_names.append(x.id)
        if tape is not None:
            if x.requires_grad:
                requires_grad = True
            elif tape.is_watched(x):
                requires_grad = True

    if tape and tape._retain_graph:
        requires_grad = True

    # Allocate outputs.
    ws = workspace.get_workspace()
    output_scope = context.get_eager_scope(requires_grad)
    gc = ws.collectors  # Garbage collectors

    for i, spec in enumerate(outputs):
        if isinstance(spec, six.string_types):
            output_names.append(spec)
        else:
            if isinstance(spec, device_spec.DeviceSpec):
                output_id = gc.TENSOR.alloc(output_scope)
                ref = EagerTensor(device=spec)
                ref.__gc__, ref._id = gc.TENSOR, output_id
                ref._impl = ws.CreateTensor(output_id)
                outputs[i] = ref
            output_names.append(outputs[i].id)

    # Generate the OpDef.
    op_def = op_def.DeriveTo(input_names, output_names)

    # Maybe record this operation for future developments.
    if len(inputs) > 0 and no_grad is False:
        if requires_grad:
            for output in outputs:
                output.requires_grad = True
            op_def.name = gc.OPERATOR.alloc(op_def.type)
            tape.add_def(op_def)
        else:
            for output in outputs:
                output.requires_grad = False

    # Dispatch the computation.
    if pre_callback is not None:
        pre_callback(op_def.name)
    workspace.run_operator(op_def)

    # Return the outputs.
    return outputs if len(outputs) > 1 else outputs[0]
