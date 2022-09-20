@echo off
call "%VS141COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x86
cd scintilla\win32
nmake -fscintilla.mak SUPPORT_XP=1
cd ..\..
pause
