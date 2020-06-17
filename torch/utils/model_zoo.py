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
#    <https://github.com/pytorch/pytorch/blob/master/torch/utils/model_zoo.py>
#
# ------------------------------------------------------------

import hashlib
import os
import re
import shutil
import sys
import tempfile

from dragon.vm import torch

try:
    from requests.utils import urlparse
    import requests.get as urlopen
    requests_available = True
except ImportError:
    requests_available = False
    if sys.version_info[0] == 2:
        from urlparse import urlparse
        from urllib2 import urlopen
    else:
        from urllib.request import urlopen
        from urllib.parse import urlparse
try:
    from tqdm import tqdm
except ImportError:
    tqdm = None

# Match bfd8deac from resnet18-bfd8deac.pth
HASH_REGEX = re.compile(r'-([a-f0-9]*)\.')


def load_url(url, model_dir=None, map_location=None, progress=True):
    if model_dir is None:
        torch_home = os.path.expanduser(os.getenv('TORCH_HOME', '~/.torch'))
        model_dir = os.getenv('TORCH_MODEL_ZOO', os.path.join(torch_home, 'models'))
    if not os.path.exists(model_dir):
        os.makedirs(model_dir)
    parts = urlparse(url)
    filename = os.path.basename(parts.path)
    cached_file = os.path.join(model_dir, filename)
    if not os.path.exists(cached_file):
        sys.stderr.write('Downloading: "{}" to {}\n'.format(url, cached_file))
        hash_prefix = HASH_REGEX.search(filename).group(1)
        _download_url_to_file(url, cached_file, hash_prefix, progress=progress)
    return torch.load(cached_file, map_location=map_location)


def _download_url_to_file(url, dst, hash_prefix, progress):
    u = urlopen(url)
    if requests_available:
        file_size = int(u.headers["Content-Length"])
        u = u.raw
    else:
        meta = u.info()
        if hasattr(meta, 'getheaders'):
            file_size = int(meta.getheaders("Content-Length")[0])
        else:
            file_size = int(meta.get_all("Content-Length")[0])

    f = tempfile.NamedTemporaryFile(delete=False)
    try:
        sha256 = hashlib.sha256()
        with tqdm(total=file_size, disable=not progress) as pbar:
            while True:
                buffer = u.read(8192)
                if len(buffer) == 0:
                    break
                f.write(buffer)
                sha256.update(buffer)
                pbar.update(len(buffer))

        f.close()
        digest = sha256.hexdigest()
        if digest[:len(hash_prefix)] != hash_prefix:
            raise RuntimeError('invalid hash value (expected "{}", got "{}")'
                               .format(hash_prefix, digest))
        shutil.move(f.name, dst)
    finally:
        f.close()
        if os.path.exists(f.name):
            os.remove(f.name)


if tqdm is None:
    class tqdm(object):

        def __init__(self, total, disable=False):
            self.total = total
            self.disable = disable
            self.n = 0

        def update(self, n):
            if self.disable:
                return

            self.n += n
            sys.stderr.write("\r{0:.1f}%".format(100 * self.n / float(self.total)))
            sys.stderr.flush()

        def __enter__(self):
            return self

        def __exit__(self, exc_type, exc_val, exc_tb):
            if self.disable:
                return

            sys.stderr.write('\n')
