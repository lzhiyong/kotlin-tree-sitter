#!/usr/bin/env python
#
# Copyright © 2023 Github Lzhiyong
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# pylint: disable=not-callable, line-too-long, no-else-return


import os
import shutil
import subprocess

from pathlib import Path

# languages
projects = {
    'tree-sitter' : 'https://github.com/tree-sitter/tree-sitter',
    'tree-sitter-c' : 'https://github.com/tree-sitter/tree-sitter-c'
}

def main():
    
    # c/c++ source
    source = Path('lib/build/src')
    
    for name in projects.keys():
        if not source.joinpath(name).exists():
            # clone the projects
            subprocess.run('git clone --depth 1 --recursive {src} {target}'.format(src = projects[name], target = source/name), shell=True)
    
    # native libs
    native = Path('lib/build/native')
    if not native.exists():
        native.mkdir()
    
    # work jobs
    jobs = os.cpu_count()
    if jobs is None:
        jobs = 16 
        
    # tree-sitter-c
    subprocess.run('clang -fPIC -shared -Ilib/build/src/tree-sitter-c/tree_sitter lib/build/src/tree-sitter-c/src/parser.c -o lib/build/native/libtree-sitter-c.so', shell=True)
    
    # tree-sitter
    subprocess.run('make -j{} -C lib/build/src/tree-sitter'.format(jobs), shell=True)
    shutil.copy2(Path('lib/build/src/tree-sitter/libtree-sitter.so.0.0'), Path('lib/build/native/libtree-sitter.so'))
    
    # tree-sitter-jni
    subprocess.run('cmake -GNinja -S lib/src/main/cpp -B lib/build/cmake/cxx', shell=True)
    subprocess.run('ninja -j{} -C lib/build/cmake/cxx'.format(jobs), shell=True)
    shutil.copy2(Path('lib/build/cmake/cxx/libtree-sitter-jni.so'), Path('lib/build/native/libtree-sitter-jni.so'))
    
if __name__ == '__main__':
    main()

