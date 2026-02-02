
cl /c main.cpp ^
     engine/tools/map_parser.cpp ^
     player/player.cpp ^
     engine/rendering/renderer.cpp /I "%CD%\SDL2\include" /I "%CD%\GL" /I "%CD%\AL"

link main.obj map_parser.obj renderer.obj player.obj imgui_tables.obj imgui_widgets.obj imgui.obj ^
    imgui_draw.obj imgui_impl_opengl3.obj imgui_impl_sdl2.obj ^
    /SUBSYSTEM:CONSOLE ^
    /LIBPATH:"%CD%\SDL2\lib\x64" ^
    /LIBPATH:"%CD%\GL\x64" ^
    /LIBPATH:"%CD%\AL\x64" ^
    SDL2main.lib SDL2.lib Shell32.lib glew32.lib glew32s.lib opengl32.lib OpenAL32.lib^
    /OUT:Bloody-Path.exe

Bloody-Path.exe
