# ------------------------------------------------------------
# Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
#
# Licensed under the BSD 2-Clause License.
# You should have received a copy of the BSD 2-Clause License
# along with the software. If not, See,
#
#    <https://opensource.org/licenses/BSD-2-Clause>
#
# Codes are based on:
#
#    <https://github.com/pytorch/pytorch/blob/master/torch/optim/sgd.py>
#
# ------------------------------------------------------------

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from dragon.vm.torch.optim.optimizer import Optimizer
from dragon.vm.torch.optim.optimizer import required


class SGD(Optimizer):
    r"""The optimizer which implements SGD algorithm.

    Following SGD algorithms are supported:

    **VanillaSGD**, whose update is defined as:

    .. math:: \text{VanillaSGD}(g) = -\text{lr} * g

    **MomentumSGD**
    `[Polyak, 1964] <https://doi.org/10.1016/0041-5553(64)90137-5>`_,
    whose update is defined as:

    .. math:: \text{MomentumSGD}(g) =
            -(\text{momentum} * m_{t-1} + \text{lr} * g)

    **NesterovSGD**
    `[Sutskever et.al, 2013] <http://www.cs.toronto.edu/~hinton/absps/momentum.pdf>`_,
    whose update is defined as:

    .. math:: \text{NesterovSGD}(g) =
            -((1 + \text{momentum}) * m_{t} - \text{momentum} * m_{t-1}) \\
        \quad \\ \text{where} \quad
            m_{t} = \text{momentum} * m_{t-1} + \text{lr} * g

    You can use one of them by setting the defaults:

    ```python
    # Set the ``lr`` only
    vanilla_sgd = torch.optim.SGD(lr=0.1)

    # Set the ``lr`` and ``momentum``
    momentum_sgd = torch.optim.SGD(lr=0.1, momentum=0.9)

    # Set the ``lr``, ``momentum`` and ``nesterov``
    nesterov_sgd = torch.optim.SGD(lr=0.1, momentum=0.9, nesterov=True)
    ```

    """

    def __init__(
        self,
        params,
        lr=required,
        momentum=0,
        dampening=0,
        weight_decay=-1.,
        nesterov=False,
        scale_gradient=1.,
        clip_gradient=-1.,
    ):
        r"""Create a ``SGD`` optimizer.

        Parameters
        ----------
        params : Sequence[dragon.vm.torch.nn.Parameter]
            The parameters to optimize.
        lr : float, required
            The initial value for :math:`\text{lr}`.
        momentum : float, optional, default=0
            The initial value for :math:`\text{momentum}`.
        dampening : float, optional, default=0
            The dampening for :math:`\text{momentum}`.
        weight_decay : float, optional, default=-1.
            The factor of L2 penalty.
        nesterov : bool, optional, default=False
            **True** to switch to **NesterovSGD** optimizer.
        scale_gradient : float, optional, default=1.
            The factor to scale gradients.
        clip_gradient : float, optional, default=-1.
            The norm thresh to clip gradients.

        """
        if lr is not required and lr < 0.:
            raise ValueError("Invalid learning rate: {}".format(lr))
        if momentum < 0.:
            raise ValueError("Invalid momentum value: {}".format(momentum))
        defaults = dict(
            lr=lr,
            momentum=momentum,
            dampening=dampening,
            weight_decay=weight_decay,
            nesterov=nesterov,
            scale_gradient=scale_gradient,
            clip_gradient=clip_gradient,
        )
        if nesterov and (momentum <= 0. or dampening != 0.):
            raise ValueError("Nesterov momentum requires a momentum and zero dampening.")
        super(SGD, self).__init__(params, defaults)
        self._update_op_type = 'NesterovUpdate' if nesterov else 'SGDUpdate'
        self._shared_args = {
            'lr': 'base_lr',
            'momentum': 'momentum',
            'weight_decay': 'l2_decay',
            'clip_gradient': 'clip_gradient',
            'scale_gradient': 'scale_gradient',
        }
