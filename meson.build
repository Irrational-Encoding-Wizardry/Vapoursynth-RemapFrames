project('RemapFrames',
        'cpp',
        default_options : ['buildtype=release', 'c_std=c11', 'cpp_std=c++11'],
        license : 'BSD-2-Clause',
        version : '1')


# Dependencies
vapoursynth = dependency('vapoursynth')


# Sources
src = [
    'Common.cpp',
    'Common.h',
    'RemapFrames.cpp',
    'RemapFramesSimple.cpp',
    'ReplaceFramesSimple.cpp',
    'VSPlugin.cpp']


# Libs
library(
    'remapframes',
    src,
    dependencies : [vapoursynth],
    install_dir : join_paths(get_option('prefix'), get_option('libdir'), 'vapoursynth'),
    install : true
)
