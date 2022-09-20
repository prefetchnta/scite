@echo off
call "%VS141COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x86
cd lexilla\src
nmake -flexilla.mak SUPPORT_XP=1
cd ..\..\scintilla\win32
nmake -fscintilla.mak SUPPORT_XP=1
cd ..\..\scite\win32
nmake -fscite.mak SUPPORT_XP=1
cd ..\..
pause
