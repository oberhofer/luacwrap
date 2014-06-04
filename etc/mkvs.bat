@rem Script to build LuaCWrap under "Visual Studio .NET Command Prompt".
@rem Do not run from this directory; run it from the toplevel: etc\mkvs.bat.
@rem It creates luacwrap.dll, luacwrap.lib and testluacwrap.dll in src.

@setlocal enableextensions enabledelayedexpansion
@ echo off

@rem Determine Lua installation directory from a list of possible locations
@rem and try the first which exists
set DIRS=%LUA_DEV%;c:\Lua\5.1;%ProgramFiles%\Lua\5.1;
set DIRS=%DIRS:;=" "%

for %%k IN ("%DIRS%") do (
  set LUAROOT=%%~k
  if EXIST !LUAROOT! goto start
)
echo "Lua installation path not found"
goto end

:start
@echo on
@echo Lua installation path: %LUAROOT%
@echo off
@set LUAINCLUDE=%LUAROOT%/include
@set LUALIB=%LUAROOT%/lib

@set MYCOMPILE=cl /nologo /MD /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE /I%LUAINCLUDE%
@set MYLINK=link /nologo /LIBPATH:%LUALIB%
@set MYMT=mt /nologo

cd src

@rem create luacwrap.dll
%MYCOMPILE% luaaux.c luacwrap.c wrapnumeric.c testluacwrap.c defconstants.c
%MYLINK% /DLL /out:luacwrap.dll /DEF:luacwrap.def luaaux.obj luacwrap.obj wrapnumeric.obj defconstants.obj lua5.1.lib
if exist luacwrap.dll.manifest^
  %MYMT% -manifest luacwrap.dll.manifest -outputresource:luacwrap.dll;2
  
@rem create testluacwrap.dll
%MYLINK% /DLL /out:testluacwrap.dll /DEF:testluacwrap.def luaaux.obj testluacwrap.obj lua5.1.lib luacwrap.lib
if exist testluacwrap.dll.manifest^
  %MYMT% -manifest testluacwrap.dll.manifest -outputresource:testluacwrap.dll;2

@rem cleanup
del *.obj *.manifest
cd ..

:end
@echo on
