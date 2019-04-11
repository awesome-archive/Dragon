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

"""We move the Module & Parameter to ``torch`` instead of ``torch.nn``,

as it will be reused by the ``torch.ops``.

"""

from dragon.vm.torch.module import Module
from dragon.vm.torch.tensor import Parameter

from .modules.conv import Conv2d, ConvTranspose2d
from .modules.depthwise_conv import DepthwiseConv2d
from .modules.pooling import MaxPool2d, AvgPool2d
from .modules.linear import Linear

from .modules.activation import (
    ReLU, LeakyReLU, ELU, SELU,
    Tanh, Sigmoid, Softmax,
)

from .modules.loss import (
    BCEWithLogitsLoss, SCEWithLogitsLoss,
    NLLLoss, CrossEntropyLoss,
    L1Loss, MSELoss, SmoothL1Loss,
    SigmoidFocalLoss, SoftmaxFocalLoss,
)

from .modules.rnn import (
    RNNBase, RNNCellBase,
    RNN, LSTM, GRU,
    LSTMCell,
)

from .modules.container import Container, Sequential, ModuleList
from .modules.batchnorm import BatchNorm1d, BatchNorm2d, BatchNorm3d
from .modules.groupnorm import GroupNorm1d, GroupNorm2d, GroupNorm3d
from .modules.affine import Affine
from .modules.dropout import Dropout, Dropout2d, Dropout3d
from .modules.dropblock import DropBlock2d
from . import init