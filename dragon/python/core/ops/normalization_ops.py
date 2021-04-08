# ------------------------------------------------------------
# Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
#
# Licensed under the BSD 2-Clause License.
# You should have received a copy of the BSD 2-Clause License
# along with the software. If not, See,
#
#     <https://opensource.org/licenses/BSD-2-Clause>
#
# ------------------------------------------------------------
"""Normalization ops."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from dragon.core import distributed
from dragon.core.autograph import context
from dragon.core.autograph.op_impl import OpLib
from dragon.core.autograph.op_impl import OpSchema


@OpSchema.num_inputs(5)
@OpSchema.convert_arg('momentum', as_target=False)
def batch_norm(
    inputs,
    axis=-1,
    momentum=0.9,
    epsilon=1e-5,
    use_stats=-1,
    **kwargs
):
    r"""Apply the batch normalization.
    `[Ioffe & Szegedy, 2015] <https://arxiv.org/abs/1502.03167>`_.

    The normalization is defined as:

    .. math:: y = \frac{x - \mathrm{E}[x]}
                       {\sqrt{\mathrm{Var}[x] + \epsilon}}
                  * \gamma + \beta

    The running average of statistics are calculated as:

    .. math:: x_{\text{running}} = \text{momentum} * x_{\text{running}}
                                   + (1 - \text{momentum}) * x_{\text{batch}}

    Parameters
    ----------
    inputs : Sequence[dragon.Tensor]
        The tensor ``x``, ``gamma``, ``beta``, ``mean`` and ``var``.
    axis : int, optional, default=-1
        The channel axis.
    momentum : Union[float, dragon.Tensor], optional
        The value to :math:`\text{momentum}`.
    epsilon : float, optional, default=1e-5
        The value to :math:`\epsilon`.
    use_stats : int, optional, default=-1
        Whether to use estimated statistics or not.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    args = OpSchema.parse_args(locals())
    args['epsilon'] = float(epsilon)
    if context.executing_eagerly():
        return OpLib.execute(
            'BatchNorm', inputs, axis=axis, epsilon=args['epsilon'],
            use_stats=use_stats, momentum=args['momentum'])
    return OpLib.add('BatchNorm', **args)


@OpSchema.num_inputs(3)
def group_norm(inputs, axis=-1, group=0, epsilon=1e-5, **kwargs):
    r"""Apply the group normalization.
    `[Wu & He, 2018] <https://arxiv.org/abs/1803.08494>`_.

    The normalization is defined as:

    .. math:: y = \frac{x - \mathrm{E}[x]}
                       {\sqrt{\mathrm{Var}[x] + \epsilon}}
                  * \gamma + \beta

    :attr:`group` could be zero to apply the instance normalization:

    ```python
    gamma, beta = dragon.ones((3,)), dragon.zeros((3,))
    x = dragon.constant([[1., 2., 3.], [4., 5., 6.]], dtype=gamma.dtype)
    y = dragon.nn.group_norm([x, gamma, beta], group=0)
    print(y)  # [[0., 0., 0.], [0., 0., 0.]]
    ```

    Parameters
    ----------
    inputs : Sequence[dragon.Tensor]
        The tensor ``x``, ``gamma`` and ``beta``.
    axis : int, optional, default=-1
        The channel axis.
    group : int, optional, default=0
        The group size.
    epsilon : float, optional, default=1e-5
        The value to :math:`\epsilon`.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    epsilon = float(epsilon)
    if context.executing_eagerly():
        return OpLib.execute(
            'GroupNorm', inputs, axis=axis, group=group, epsilon=epsilon)
    return OpLib.add('GroupNorm', inputs, axis=axis,
                     group=group, epsilon=epsilon, **kwargs)


@OpSchema.num_inputs(3)
def instance_norm(inputs, axis=-1, epsilon=1e-5, **kwargs):
    r"""Apply the instance normalization.
    `[Ulyanov et.al, 2016] <https://arxiv.org/abs/1607.08022>`_

    The normalization is defined as:

    .. math:: y = \frac{x - \mathrm{E}[x]}
                       {\sqrt{\mathrm{Var}[x] + \epsilon}}
                  * \gamma + \beta

    Parameters
    ----------
    inputs : Sequence[dragon.Tensor]
        The tensor ``x``, ``gamma`` and ``beta``.
    axis : int, optional, default=-1
        The channel axis.
    epsilon : float, optional, default=1e-5
        The value to :math:`\epsilon`.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    return group_norm(inputs, axis=axis, group=0, epsilon=epsilon, **kwargs)


@OpSchema.num_inputs(3)
def layer_norm(inputs, axis=-1, epsilon=1e-5, **kwargs):
    r"""Apply the layer normalization.
    `[Ba et.al, 2016] <https://arxiv.org/abs/1607.06450>`_

    The normalization is defined as:

    .. math:: y = \frac{x - \mathrm{E}[x]}
                       {\sqrt{\mathrm{Var}[x] + \epsilon}}
                  * \gamma + \beta

    Parameters
    ----------
    inputs : Sequence[dragon.Tensor]
        The tensor ``x``, ``gamma`` and ``beta``.
    axis : int, optional, default=-1
        The start axis of normalized dimensions.
    epsilon : float, optional, default=1e-5
        The value to :math:`\epsilon`.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    epsilon = float(epsilon)
    if context.executing_eagerly():
        return OpLib.execute(
            'LayerNorm', inputs, axis=axis, epsilon=epsilon)
    return OpLib.add('LayerNorm', inputs, axis=axis, epsilon=epsilon, **kwargs)


@OpSchema.num_inputs(1)
def lp_normalize(
    inputs,
    axis=-1,
    end_axis=None,
    p=2,
    epsilon=1e-12,
    reduction='sum',
    **kwargs
):
    r"""Apply the lp normalization.

    The normalization is defined as:

    .. math:: y = \frac{x}{\max(\left\|x\right\|_{p}, \epsilon)}

    :attr:`axis` could be negative:

    ```python
    x = dragon.constant([[1, 2, 3], [4, 5, 6]], 'float32')
    # A negative axis is the last-k axis
    print(dragon.math.lp_normalize(x, 1))
    print(dragon.math.lp_normalize(x, -1))  # Equivalent
    ```

    More than one axis could be specified to reduce:

    ```python
    # Along the continuous axes: [axis, end_axis]
    print(dragon.math.lp_normalize(x, axis=0, end_axis=1))
    ```

    Parameters
    ----------
    inputs : dragon.Tensor
        The tensor :math:`x`.
    p : int, optional, default=2
        The order of the normalization.
    axis : int, optional, default=-1
        The first axis to reduce.
    end_axis : int, optional
        The last axis to reduce.
    epsilon : float, optional, default=1e-12
        The value to :math:`\epsilon`.
    reduction : {'sum', 'mean'}, optional
        The reduction method for norm.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    epsilon = float(epsilon)
    reduction = reduction.upper()
    if context.executing_eagerly():
        return OpLib.execute(
            'LpNormalize', inputs, p=p, axis=axis, end_axis=end_axis,
            epsilon=epsilon, reduction=reduction)
    return OpLib.add('LpNormalize', inputs, p=p, axis=axis, end_axis=end_axis,
                     epsilon=epsilon, reduction=reduction, **kwargs)


@OpSchema.num_inputs(1)
def local_response_norm(
    inputs,
    size=5,
    alpha=0.0001,
    beta=0.75,
    bias=1.,
    data_format='NCHW',
    **kwargs
):
    r"""Apply the local response normalization.
    `[Krizhevsky et.al, 2012] <http://www.cs.toronto.edu/~hinton/absps/imagenet.pdf>`_.

    The normalization is defined as:

    .. math::
        y_{i} = x_{i}\left(k + \frac{\alpha}{n}
                     \sum_{j=\max(0, i-n/2)}^{\min(N-1,i+n/2)}x_{j}^2
                     \right)^{-\beta}

    Parameters
    ----------
    inputs : dragon.Tensor
        The input tensor.
    size : int, optional, default=5
        The number of neighbouring channels to sum over.
    alpha : float, optional, default=0.0001
        The scale value :math:`\alpha`.
    beta : float, optional, default=0.75
        The exponent value :math:`\beta`.
    bias : float, optional, default=1.
        The bias constant :math:`k`.
    data_format : str, optional, default='NCHW'
        ``'NCHW'`` or ``'NHWC'``.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    args = OpSchema.parse_args(locals())
    if data_format not in ('NCHW', 'NHWC'):
        raise ValueError('Unsupported data format: %s' % data_format)
    args['alpha'] = float(alpha)
    args['beta'] = float(beta)
    args['bias'] = float(bias)
    if context.executing_eagerly():
        op = op_lib.create(
            'LRN', size=size, alpha=args['alpha'], beta=args['beta'],
            bias=args['bias'], data_format=data_format)
        return op.execute([inputs])
    return op_lib.symbolize('LRN', **args)


@OpSchema.num_inputs(min_num=5, max_num=5)
@OpSchema.convert_arg('momentum', as_target=False)
def sync_batch_norm(
    inputs,
    axis=-1,
    momentum=0.9,
    epsilon=1e-5,
    use_stats=-1,
    process_group=None,
    **kwargs
):
    r"""Apply the batch normalization with synced statistics.
    `[Ioffe & Szegedy, 2015] <https://arxiv.org/abs/1502.03167>`_.

    The normalization is defined as:

    .. math:: y = \frac{x - \mathrm{E}[x]}
                       {\sqrt{\mathrm{Var}[x] + \epsilon}}
                  * \gamma + \beta

    The running average of statistics are calculated as:

    .. math:: x_{\text{running}} = \text{momentum} * x_{\text{running}}
                                   + (1 - \text{momentum}) * x_{\text{batch}}

    Parameters
    ----------
    inputs : Sequence[dragon.Tensor]
        The tensor ``x``, ``gamma``, ``beta``, ``mean`` and ``var``.
    axis : int, optional, default=-1
        The channel axis.
    momentum : Union[float, dragon.Tensor], optional
        The value to :math:`\text{momentum}`.
    epsilon : float, optional, default=1e-5
        The value to :math:`\epsilon`.
    use_stats : int, optional, default=-1
        Whether to use estimated statistics or not.
    process_group : ProcessGroup, optional
        The group for communication.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    args = OpSchema.parse_args(locals())
    args['epsilon'] = float(epsilon)
    if process_group is None:
        process_group = distributed.get_group()
    if process_group is None:
        raise ValueError('<process_group> is required.')
    if context.executing_eagerly():
        op = op_lib.create(
            'SyncBatchNorm', axis=axis, epsilon=args['epsilon'],
            use_stats=use_stats, process_group=process_group)
        return op.execute(inputs, momentum=args['momentum'])
    args.update(process_group.arguments)
    return op_lib.symbolize('SyncBatchNorm', **args)
