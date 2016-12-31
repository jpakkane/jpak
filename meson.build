project('jpack', 'cpp',
  version : '0.0.1',
  license : 'GPLv3+',
  default_options : ['cpp_std=c++14', 'warning_level=3'])

lzma_dep = dependency('liblzma')

lib = static_library('helpers', 'fileutils.cpp', 'utils.cpp', 'file.cpp', 'mmapper.cpp',
  dependencies : lzma_dep)

executable('jpack', 'jpack.cpp', 'jpacker.cpp', link_with : lib)
executable('junpack', 'junpack.cpp', link_with : lib)

