@echo off

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cl /c main.cpp ^
     engine/tools/map_parser.cpp ^
     engine/tools/map_renderer.cpp ^
     engine/tools/sprite_sampler.cpp ^
     player/player.cpp ^
     engine/rendering/renderer.cpp /I "%CD%\SDL2\include" /I "%CD%\GL" /I "%CD%\AL"

link main.obj ^
    player.obj ^
    map_parser.obj ^
    renderer.obj ^
    sprite_sampler.obj ^
    map_renderer.obj ^
    imgui_tables.obj ^
    imgui_widgets.obj ^
    imgui.obj ^
    imgui_draw.obj imgui_impl_opengl3.obj imgui_impl_sdl2.obj ^
    /SUBSYSTEM:CONSOLE ^
    /LIBPATH:"%CD%\SDL2\lib\x64" ^
    /LIBPATH:"%CD%\GL\x64" ^
    /LIBPATH:"%CD%\AL\x64" ^
    SDL2main.lib SDL2.lib Shell32.lib glew32.lib glew32s.lib opengl32.lib OpenAL32.lib^
    /OUT:Bloody-Path.exe

@REM Bloody-Path.exe
