@echo off
call "%VS141COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x86
cd scite\win32
nmake -fscite.mak SUPPORT_XP=1
cd ..\..
pause
