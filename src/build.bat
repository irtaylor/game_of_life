@echo off

set CommonCompilerFlags=-wd4505 -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC -Z7 -DGOL_DEBUG=1
set CommonLinkerFlags= -opt:ref user32.lib gdi32.lib

IF NOT EXIST w:\game_of_life\build mkdir w:\game_of_life\build
pushd w:\game_of_life\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
cl %CommonCompilerFlags% ..\src\win32_life.cpp /link %CommonLinkerFlags%
popd
