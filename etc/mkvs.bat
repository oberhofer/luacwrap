@rem Script to build LuaCWrap under "Visual Studio .NET Command Prompt".
@rem Do not run from this directory; run it from the toplevel: etc\mkvs.bat.
@rem It creates luacwrap.dll, luacwrap.lib and testluacwrap.dll in src.

@setlocal enableextensions enabledelayedexpansion
rem @ echo off

set LUA_SVER=52
set COMPATFLAG=


@rem Determine Lua installation directory from a list of possible locations
@rem and try the first which exists
set DIRS=c:\Lua;%ProgramFiles%\Lua;%LUA_DEV%
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

if %LUA_SVER%==51 ( 
  set LUAINCLUDE=%LUAROOT%/include/lua/5.1
  set LUALIBPATH=%LUAROOT%/lib/
  set LUALIB=lua51.lib
) 
if %LUA_SVER%==52 ( 
  set LUAINCLUDE=%LUAROOT%/include/lua/5.2
  set LUALIBPATH=%LUAROOT%/lib/
  set LUALIB=lua52.lib
  if "%COMPATFLAG%"=="" (
    set COMPATFLAG=-DLUA_COMPAT_ALL
  )
) 
if %LUA_SVER%==53 ( 
  set LUAINCLUDE=%LUAROOT%/include/lua/5.3
  set LUALIBPATH=%LUAROOT%/lib/
  set LUALIB=lua53.lib
  if "%COMPATFLAG%"=="" (
    set COMPATFLAG=-DLUA_COMPAT_5_2
  )
) 

@echo Lua installation path: %LUAROOT%

rem @set MYCOMPILE=cl /nologo /MD /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE /I%LUAINCLUDE% /I../include
@set MYCOMPILE=cl /nologo /MD /Zi /W3 /c /D_CRT_SECURE_NO_DEPRECATE /I%LUAINCLUDE% /I../include
@set MYLINK=link /nologo /DEBUG /LIBPATH:%LUALIBPATH%
@set MYMT=mt /nologo

cd src

@rem create luacwrap.dll
%MYCOMPILE% /Fdluacwrap.pdb luaaux.c luacwrap.c wrapnumeric.c testluacwrap.c defconstants.c
%MYLINK% /DLL /pdb:luacwrap.pdb /out:luacwrap.dll /DEF:luacwrap.def luaaux.obj luacwrap.obj wrapnumeric.obj defconstants.obj %LUALIB%
if exist luacwrap.dll.manifest^
  %MYMT% -manifest luacwrap.dll.manifest -outputresource:luacwrap.dll;2
  
@rem create testluacwrap.dll
%MYCOMPILE% /Fdtestluacwrap.pdb testluacwrap.c 
%MYLINK% /DLL /pdb:testluacwrap.pdb /out:testluacwrap.dll /DEF:testluacwrap.def luaaux.obj testluacwrap.obj luacwrap.lib %LUALIB%
if exist testluacwrap.dll.manifest^
  %MYMT% -manifest testluacwrap.dll.manifest -outputresource:testluacwrap.dll;2

@rem cleanup
del *.obj *.manifest
cd ..

:end
@echo on
