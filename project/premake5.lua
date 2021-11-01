--
-- premake5 script (last used 5.0.0alpha16)
-- see https://github.com/premake/premake-core/wiki for details
--
-- e.g. create vs2019 project files
-- premake5.exe --file=premake5.lua --os=windows vs2019
--

-------------------------------------------------------------------------------

solution "luacwrap"
  configurations { "Debug", "Release" }
  platforms { "Win32", "x64" }

  -- 
  --  common settings applied to all projects
  --
  function stdSettings(basepath)
    
    defines { "_CRT_SECURE_NO_WARNINGS" }
    
    symbols "On"
    symbolspath "$(OutDir)\\$(TargetName).pdb"
   
    filter "platforms:*32"
      architecture "x86"
      libdirs {  "$(LUA)/bin/x32/$(Configuration)", 
                 "..\\bin\\x32\\$(Configuration)"
                 }
      targetdir "..\\bin\\x32\\$(Configuration)"

    filter "platforms:*64"
      architecture "x86_64"
      libdirs {  "$(LUA)/bin/x64/$(Configuration)",
                 "..\\bin\\x64\\$(Configuration)"
              }
      targetdir "..\\bin\\x64\\$(Configuration)"
    
    filter "configurations:Debug"
      defines { "DEBUG" }
      -- flags { "Symbols", "Unicode" }
      symbols "On"
      characterset "Unicode"
      -- Perform a static link against the standard runtime libraries
      staticruntime "On"

    filter "configurations:Release"
      defines { "NDEBUG" }
      characterset "Unicode"
      symbols "On"
      optimize "On"
      -- Perform a static link against the standard runtime libraries
      staticruntime "On"
  end

  --=======================================================================
  basepath = [[../src/]]
  
  project "luacwrap"
    kind "SharedLib"
    language "C++"

    defines {
    }

    files 
    { 
      "../include/*.h", 
      basepath .. "luacwrap.def", 
      basepath .. "defconstants.c", 
      basepath .. "luacwrap.c",
      basepath .. "luaaux.h",
      basepath .. "luaaux.c", 
      basepath .. "wrapnumeric.h",
      basepath .. "wrapnumeric.c", 
      basepath .. "wrappointer.h",
      basepath .. "wrappointer.c",
      basepath .. "wrapreference.h",
      basepath .. "wrapreference.c",
      basepath .. "luacwrap_int.h",
    }
    
    includedirs { 
      "../include",
      "../src",
      basepath,
      [[c:\Lua\5.1\include]],
    }

    libdirs {  
      [[c:\Lua\5.1\lib]],
    }
    
    resincludedirs { 
    }

    links { 
      "lua51.lib",
    }

    stdSettings(basepath)

  --=======================================================================
  basepath = [[../src/]]
  
  project "testluacwrap"
    kind "SharedLib"
    language "C++"

    defines {
    }

    files 
    { 
      "../include/*.h", 
      basepath .. "testluacwrap.def", 
      basepath .. "testluacwrap.c", 
      basepath .. "luaaux.c", 
    }
    
    includedirs { 
      "../include",
      "../src",
      basepath,
      [[c:\Lua\5.1\include]],
    }

    libdirs {  
      [[c:\Lua\5.1\lib]],
    }
    
    resincludedirs { 
    }

    links { 
      "lua51.lib",
      "luacwrap",
    }

    stdSettings(basepath)
