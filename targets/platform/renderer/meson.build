cpp_args = global_cpp_args + global_cpp_defs

if host_machine.system() == 'darwin'
  cpp_args += ['-xobjective-c++']
endif

lib_platform_renderer_bgfx = static_library('platform_renderer',
    files('bgfx/BgfxRenderPath.cpp', 'bgfx/gl_stubs.cpp', 'RenderPathGlobals.cpp'),
    include_directories: include_directories('../../'),
    dependencies: [
        dependency('bgfx'),
        dependency('sdl2'),
        dependency('glm'),
        dependency('stb'),
        java_dep
    ],
    cpp_args: cpp_args,
)

platform_renderer_bgfx_dep = declare_dependency(
    link_with: lib_platform_renderer_bgfx,
)
