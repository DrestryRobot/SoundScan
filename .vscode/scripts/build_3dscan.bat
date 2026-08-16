@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" || exit /b 1
set "BUILD=%~dp0..\..\3DScan\build\release"
if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%"
"C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe" "%~dp0..\..\3DScan\3DScan.pro" -spec win32-msvc CONFIG+=release || exit /b 1
"C:\Qt\Tools\QtCreator\bin\jom\jom.exe" -j%NUMBER_OF_PROCESSORS%
exit /b %ERRORLEVEL%
