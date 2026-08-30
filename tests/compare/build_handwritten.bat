@echo off
rem Builds tree_handwritten.exe against the Release LogoCore static lib.
rem Adjust the vcvars64 path for your VS edition (BuildTools/Community/Pro),
rem or run this script from a developer prompt where cl is already on PATH.
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d %~dp0..\..
cl /nologo /O2 /MD /EHsc /std:c++20 /I LogoCore tests\compare\tree_handwritten.cpp /Fe:tests\compare\handwritten.exe /link x64\Release\LogoCore.lib
