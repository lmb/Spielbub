solution "Spielbub"
   configurations { "Debug", "Release" }
   includedirs { "/usr/local/include", "include" }
   libdirs { "/usr/local/lib" }

   flags { "FatalWarnings" }

   -- if not os.get() == "windows" then
      buildoptions { "-ansi", "-std=gnu11", "-pedantic", "-Wextra", }
   -- end

   configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols" }
 
   configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }

   project "Spielbub"
      kind "ConsoleApp"
      language "C"
      files { "src/debugger/*.c" }
      links { "Spiellib" }

      links { "sdl2" }

      if os.get() == "macosx" then
         links { "Cocoa.framework" }
      end

   project "tests"
      kind "ConsoleApp"
      language "C"
      files { "src/tests/*.c" }
      links { "Spiellib", "check" }

      links { "sdl2" }

      if os.get() == "macosx" then
         links { "Cocoa.framework" }
      end

   project "Spiellib"
      kind "StaticLib"
      language "C"
      files { "src/*.h", "src/*.c", "src/graphics/*.c" }