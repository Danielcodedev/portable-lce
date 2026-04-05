platform_inc = include_directories('.')
_inc_dir = '../../' / inc_dir
_threads = dependency('threads')

lib_platform_core = static_library('platform_core',
  files('C4JThread.cpp', 'ShutdownManager.cpp'),
  include_directories: [platform_inc, include_directories('..')],
  dependencies: [_threads],
  cpp_args: global_cpp_args + global_cpp_defs,
)

platform_dep = declare_dependency(
  link_with: lib_platform_core,
  include_directories: platform_inc,
)

# SDL2-based platform implementations (formerly 4J.* modules)
if host_machine.system() == 'android'
  _sdl2 = declare_dependency(
  dependencies: cc.find_library('SDL2', dirs: lib_dir, required: true ), 
  include_directories: _inc_dir)
else
  _sdl2 = dependency('sdl2')
endif
_defs = []

if get_option('renderer') == 'gles'
  if host_machine.system() == 'android'
    _gl = cc.find_library('GLESv3', required: true )
    _egl  = cc.find_library('EGL', required: true )
  else
    _gl = dependency('glesv2', required: true)
  endif
  _defs += ['-DGLES']
else
  _gl = dependency('gl', required: true)
endif

deps = [
  _sdl2, 
  _gl, 
  _threads, 
  glm_dep,
  stb_dep,
]

if host_machine.system() == 'android'
  deps += _egl
endif

sdl2_sources = files(
  'sdl2/Input.cpp',
  'sdl2/Profile.cpp',
  'sdl2/Render.cpp',
  'sdl2/render_stubs.cpp',
  'sdl2/Storage.cpp',
)

lib_platform_sdl2 = static_library('platform_sdl2',
  sdl2_sources,
  include_directories: [platform_inc, include_directories('sdl2')],
  dependencies: deps,
  cpp_args: _defs + global_cpp_args + global_cpp_defs,
)

# Combined dependency: interfaces + SDL2 implementations
# Replaces the old render_dep, input_dep, profile_dep, storage_dep
render_dep = declare_dependency(
  link_with: lib_platform_sdl2,
  include_directories: [platform_inc, include_directories('sdl2')],
  dependencies: deps,
)
input_dep = render_dep
profile_dep = render_dep
storage_dep = render_dep
