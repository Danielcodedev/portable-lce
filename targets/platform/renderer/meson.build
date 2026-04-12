bgfx_dep = dependency('bgfx', fallback: ['bgfx', 'bgfx_dep'])

lib_platform_renderer = static_library('platform_renderer',
    files('bgfx/BgfxRenderPath.cpp', 'bgfx/gl_stubs.cpp', 'RenderPathGlobals.cpp'),
    include_directories: include_directories('../../'),
    dependencies: [bgfx_dep, dependency('sdl2'), dependency('glm'), java_dep],
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_renderer_dep = declare_dependency(
    link_with: lib_platform_renderer,
)
