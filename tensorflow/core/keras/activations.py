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
#    <https://github.com/tensorflow/tensorflow/blob/master/tensorflow/python/keras/activations.py>
#
# ------------------------------------------------------------

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from dragon.core.util import six
from dragon.vm.tensorflow.core.ops import math_ops
from dragon.vm.tensorflow.core.ops import nn


def elu(x, alpha=1., **kwargs):
    r"""Apply the exponential exponential linear unit to input.
    `[Clevert et.al, 2015] <https://arxiv.org/abs/1511.07289>`_.

    The **ELU** function is defined as:

    .. math::
        \text{ELU}(x) =
        \begin{cases}
            x, & \text{ if } x \geq 0 \\
            \alpha * (e^{x} - 1), & \text{ otherwise }
        \end{cases}

    Examples:

    ```python
    x = tf.constant([-1, 0, 1], 'float32')
    print(tf.keras.activations.elu(x, inplace=False))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The tensor :math:`x`.
    alpha : float, optional, default=1.
        The value of :math:`\alpha`.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    return nn.elu(x, alpha=alpha, **kwargs)


def exponential(x):
    r"""Apply the exponential activation to input.

    The **Exponential** function is defined as:

    .. math:: \text{out} = e^{x}

    Examples:

    ```python
    x = tf.constant([1, 2, 3], 'float32')
    print(tf.keras.activations.exponential(x))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The tensor :math:`x`.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    return math_ops.exp(x)


def linear(x):
    r"""Apply the linear activation to input.

    The **Linear** function is defined as:

    .. math:: \text{Linear}(x) = x

    Examples:

    ```python
    x = tf.constant([1, 2, 3], 'float32')
    print(tf.keras.activations.linear(x))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The input tensor.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    return x


def relu(x, alpha=0, max_value=None, **kwargs):
    r"""Apply the rectified linear unit to input.
    `[Nair & Hinton, 2010] <http://www.csri.utoronto.ca/~hinton/absps/reluICML.pdf>`_.

    The **ReLU** function is defined as:

    .. math::
        \text{ReLU}(x) =
            \begin{cases}
                \min(x, v_{max}), & \text{ if } x \geq 0 \\
                \alpha * x, & \text{ otherwise }
            \end{cases}

    Examples:

    ```python
    x = tf.constant([-1, 0, 1], 'float32')
    print(tf.keras.activations.relu(x, inplace=False))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The input tensor.
    alpha : number, optional, default=0
        The valve of :math:`\alpha`.
    max_value : number, optional
        The value of :math:`v_{max}`.

    """
    if max_value is not None:
        if alpha != 0:
            raise ValueError('Set either <alpha> or <max_value>.')
        if max_value != 6:
            raise ValueError('<max_value> can only be set to 6.')
        return nn.relu6(x, **kwargs)
    return nn.leaky_relu(x, alpha=alpha, **kwargs)


def selu(x, **kwargs):
    r"""Apply the scaled exponential linear unit to input.
    `[Klambauer et.al, 2017] <https://arxiv.org/abs/1706.02515>`_.

    .. math::
        \text{SELU}(x) = 1.0507 *
        \begin{cases}
            x, & \text{ if } x \geq 0 \\
            1.67326 * (e^{x} - 1), & \text{ otherwise }
        \end{cases}

    Examples:

    ```python
    x = tf.constant([-1, 0, 1], 'float32')
    print(tf.keras.activations.selu(x, inplace=False))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The tensor :math:`x`.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    return nn.selu(x, **kwargs)


def sigmoid(x, **kwargs):
    r"""Apply the sigmoid function to input.

    The **Sigmoid** function is defined as:

    .. math:: \text{Sigmoid}(x) = \frac{1}{1 + e^{-x}}

    Examples:

    ```python
    x = tf.constant([0.2, 0.4, 0.6, 0.8, 1.0], 'float32')
    print(tf.keras.activations.sigmoid(x, inplace=False))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The input tensor.

    Returns
    -------
    dragon.Tensor
        The output tensor

    """
    return math_ops.sigmoid(x, **kwargs)


def softmax(x, axis=-1, **kwargs):
    r"""Apply the softmax function to input.

    The **Softmax** function is defined as:

    .. math:: \text{Softmax}(x) = \frac{e^{x_{i}}}{\sum e^{x_{j}}}

    Examples:

    ```python
    x = tf.constant([-1, 0, 1], 'float32')
    print(tf.keras.activations.softmax(x, inplace=False))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The tensor :math:`x`.
    axis : int, optional, default=-1
        The axis to reduce.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    return nn.softmax(x, axis=axis, **kwargs)


def tanh(x, **kwargs):
    r"""Apply the tanh function to input.

    The **Tanh** function is defined as:

    .. math:: \text{Tanh}(x) = \frac{e^{x} - e^{-x}}{e^{x} + e^{-x}}

    Examples:

    ```python
    x = tf.constant([0.2, 0.4, 0.6, 0.8, 1.0], 'float32')
    print(tf.keras.activations.tanh(x, inplace=False))
    ```

    Parameters
    ----------
    x : dragon.Tensor
        The input tensor.

    Returns
    -------
    dragon.Tensor
        The output tensor.

    """
    return math_ops.tanh(x, **kwargs)


def get(identifier):
    if identifier is None:
        return linear
    elif callable(identifier):
        return identifier
    elif isinstance(identifier, six.string_types):
        return globals()[identifier]()
    else:
        raise TypeError(
            'Could not interpret activation function identifier: {}.'
            .format(repr(identifier))
        )
