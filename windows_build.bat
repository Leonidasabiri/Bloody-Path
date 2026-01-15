
cl /c main.cpp ^
     engine/tools/map_parser.cpp ^
     player/player.cpp ^
     engine/rendering/renderer.cpp /I "%CD%\SDL2\include" /I "%CD%\GL"

link main.obj map_parser.obj renderer.obj player.obj ^
    /SUBSYSTEM:CONSOLE ^
    /LIBPATH:"%CD%\SDL2\lib\x64" ^
    /LIBPATH:"%CD%\GL\x64" ^
    SDL2main.lib SDL2.lib Shell32.lib glew32.lib glew32s.lib opengl32.lib ^
    /OUT:Bloody-Path.exe

@REM Bloody-Path.exe
