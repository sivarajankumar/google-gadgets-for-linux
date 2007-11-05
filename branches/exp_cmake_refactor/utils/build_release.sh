#!/bin/sh
#
# Copyright 2007 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e
here=`pwd`
cd `dirname $0`/..
srcdir=`pwd`
if [ ! -x cmake/build_spidermonkey.sh ]; then
  chmod +x cmake/build_spidermonkey.sh
fi
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/opt/google/ggadget" "$srcdir"
make "$@"
# sudo make install
if ! ctest . ; then
  cat Testing/Temporary/LastTest.log
fi
