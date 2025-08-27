# Copyright (C) 2025 William Henning
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

project('VkCallbackSwapchain', 'cpp',
  version: '0.0.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=1',
  ]
)

# Compiler flags (equivalent to -fno-rtti -fno-exceptions for non-MSVC)
cpp_args = []
if not (host_machine.system() == 'windows')
  cpp_args += ['-fno-rtti', '-fno-exceptions']
endif

# Dependencies
gtest_dep = dependency('gtest', main: true)

# Source files
sources = [
  'src/layer.cpp',
  'src/swapchain.cpp',
  'src/callback_swapchain.cpp',
  'src/present_callback.cpp',
  'src/shm.cpp',
  'src/shm_pixbuf_reader.cpp',
  'src/shm_pixbuf_writer.cpp',
  'src/timeout_sem.cpp',
]

# Headers (for IDE support)
headers = [
  'src/layer.h',
  'src/swapchain.h',
  'src/threading.h',
  'src/callback_swapchain.h',
  'src/present_callback.h',
  'src/generic_unique_ptr.h',
  'src/shm.h',
  'src/shm_pixbuf_data.h',
  'src/shm_pixbuf_reader.h',
  'src/shm_pixbuf_writer.h',
  'src/timeout_sem.h',
]

# Include directories
inc_dirs = ['src']

# Build the shared library
vklayer_lib = shared_library('VkLayer_Vkvfb',
  sources,
  cpp_args: cpp_args,
  include_directories: include_directories(inc_dirs),
  install: true,
  name_prefix: '',  # Remove 'lib' prefix on Unix systems
)

# Configure the JSON layer manifest
conf_data = configuration_data()
library_name = vklayer_lib.full_path().split('/')[-1]  # Gets just the filename
conf_data.set('LIBRARY_NAME', library_name)

# Generate the layer manifest JSON file
json_file = configure_file(
  input: 'VkvfbLayer.json',
  output: 'VkvfbLayer.json',
  configuration: conf_data,
  install: true,
  install_dir: get_option('libdir')
)

# Unit tests
test_sources = [
  'src/shm_pixbuf_reader_test.cpp',
  'src/shm.cpp',
  'src/shm_pixbuf_reader.cpp',
  'src/shm_pixbuf_writer.cpp',
  'src/timeout_sem.cpp',
]

test_exe = executable('shm_pixbuf_reader_test',
  test_sources,
  dependencies: gtest_dep,
  cpp_args: cpp_args,
  include_directories: include_directories(inc_dirs),
)

test('shm_pixbuf_reader_test', test_exe)

snapshot_vfb_sources = [
  'tests/snapshot_vfb.cpp',
  'src/shm.cpp',
  'src/shm_pixbuf_reader.cpp',
  'src/timeout_sem.cpp',
]

snapshot_vfb_exe = executable('snapshot_vfb',
  snapshot_vfb_sources,
  cpp_args: cpp_args,
  include_directories: include_directories(inc_dirs),
)

vfbmon_sources = [
  'tests/vfbmon.cpp',
  'src/shm.cpp',
  'src/shm_pixbuf_reader.cpp',
  'src/timeout_sem.cpp',
]

x11_dep = dependency('x11')

vfbmon_exe = executable('vfbmon',
  vfbmon_sources,
  dependencies: x11_dep,
  cpp_args: cpp_args,
  include_directories: include_directories(inc_dirs),
)
