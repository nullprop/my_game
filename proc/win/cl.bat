@REM TODO: UPDATE
@echo off
rmdir /Q /S bin
mkdir bin
pushd bin

rem Name
set name=Game

rem Include directories 
set inc=/I ..\third_party\include\

rem Source files
set src_dir=..\src

rem All source together
set src_all=%src_dir%\main.c ^
%src_dir%\audio\*.c ^
%src_dir%\bsp\*.c ^
%src_dir%\entities\*.c ^
%src_dir%\graphics\*.c ^
%src_dir%\util\*.c

rem OS Libraries
set os_libs= opengl32.lib kernel32.lib user32.lib ^
shell32.lib vcruntime.lib msvcrt.lib gdi32.lib Winmm.lib Advapi32.lib

rem Link options
set l_options=/EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib

rem Compile Release
rem cl /MP /FS /Ox /W0 /Fe%name%.exe %src_all% %inc% ^
rem /EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:LIBCMT ^
rem %os_libs%

rem Compile Debug
cl /w /MP -Zi /DEBUG:FULL /Fe%name%.exe %src_all% %inc% ^
/EHsc /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:LIBCMT ^
%os_libs%

popd

if %ERRORLEVEL% EQU 0 (
    rem Copy assets
    echo.
    echo Copying assets:
    robocopy .\assets .\bin /E /NFL /NDL /NJH /NP
    echo Copying shaders:
    robocopy .\src\shaders .\bin\shaders /E /NFL /NDL /NJH /NP
)
