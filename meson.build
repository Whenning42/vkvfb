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
x11_xcb_dep = dependency('x11-xcb')
xcb_dep = dependency('xcb')

# Source files
sources = [
  'src/layer/layer.cpp',
  'src/layer/swapchain.cpp',
  'src/layer/callback_swapchain.cpp',
  'src/layer/present_callback.cpp',
  'src/ipc/shm.cpp',
  'src/pixbuf/pixbuf_reader.cpp',
  'src/pixbuf/pixbuf_writer.cpp',
]

# Headers (for IDE support)
headers = [
  'src/layer/layer.h',
  'src/layer/swapchain.h',
  'src/layer/callback_swapchain.h',
  'src/layer/present_callback.h',
  'src/threading.h',
  'src/generic_unique_ptr.h',
  'src/logger.h',
  'src/constants.h',
  'src/utility.h',
  'src/ipc/shm.h',
  'src/ipc/pmutex.h',
  'src/ipc/fake_pmutex.h',
  'src/ipc/shm_mutex.h',
  'src/pixbuf/pixbuf_data.h',
  'src/pixbuf/pixbuf_reader.h',
  'src/pixbuf/pixbuf_writer.h',
]

# Include directories
inc_dirs = ['src']

# Build the shared library
vklayer_lib = shared_library('VkLayer_Vkvfb',
  sources,
  dependencies: [x11_xcb_dep, xcb_dep],
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
  'src/pixbuf/pixbuf_reader_test.cpp',
  'src/ipc/shm.cpp',
  'src/pixbuf/pixbuf_reader.cpp',
  'src/pixbuf/pixbuf_writer.cpp',
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
  'src/ipc/shm.cpp',
  'src/pixbuf/pixbuf_reader.cpp',
]

snapshot_vfb_exe = executable('snapshot_vfb',
  snapshot_vfb_sources,
  cpp_args: cpp_args,
  include_directories: include_directories(inc_dirs),
)

vfbmon_sources = [
  'tests/vfbmon.cpp',
  'src/ipc/shm.cpp',
  'src/pixbuf/pixbuf_reader.cpp',
]

x11_dep = dependency('x11')

vfbmon_exe = executable('vfbmon',
  vfbmon_sources,
  dependencies: x11_dep,
  cpp_args: cpp_args,
  include_directories: include_directories(inc_dirs),
)
