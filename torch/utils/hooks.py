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
#    <https://github.com/pytorch/pytorch/blob/master/torch/utils/hooks.py>
#
# ------------------------------------------------------------

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import collections
import weakref


class RemovableHandle(object):
    """A handle which provides the capability to remove a hook."""

    next_id = 0

    def __init__(self, hooks_dict):
        self.hooks_dict_ref = weakref.ref(hooks_dict)
        self.id = RemovableHandle.next_id
        RemovableHandle.next_id += 1

    def remove(self):
        hooks_dict = self.hooks_dict_ref()
        if hooks_dict is not None and self.id in hooks_dict:
            del hooks_dict[self.id]

    def __getstate__(self):
        return self.hooks_dict_ref(), self.id

    def __setstate__(self, state):
        if state[0] is None:
            # Create a dead reference.
            self.hooks_dict_ref = weakref.ref(collections.OrderedDict())
        else:
            self.hooks_dict_ref = weakref.ref(state[0])
        self.id = state[1]
        RemovableHandle.next_id = max(RemovableHandle.next_id, self.id + 1)

    def __enter__(self):
        return self

    def __exit__(self, type, value, tb):
        self.remove()
