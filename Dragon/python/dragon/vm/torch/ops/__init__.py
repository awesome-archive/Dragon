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

from .creation import (
    zeros, zeros_like, ones, ones_like,
    rand, randn
)

from .arithmetic import (
    add, sub, mul, div,
)

from .ndarray import (
    sum, mean, argmin, argmax, max, topk, cat
)