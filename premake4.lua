solution "Spielbub"
   configurations { "Debug", "Release" }
   includedirs { "/usr/local/include", "include" }
   libdirs { "/usr/local/lib" }

   flags { "FatalWarnings" }

   buildoptions { "-ansi", "-std=c2x", "-Wextra", "-Wno-gnu-case-range" }
   if os.get() == "linux" then
      buildoptions { "-D_XOPEN_SOURCE=600"}
   end

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

      if os.get() == "macosx" then
         links { "Cocoa.framework" }
      elseif os.get() == "linux" then
         links {"m"}         
      end
      links { "SDL2" }

   project "tests"
      kind "ConsoleApp"
      language "C"
      files { "src/tests/*.c" }
      links { "Spiellib", "check" }

      if os.get() == "macosx" then
         links { "Cocoa.framework" }

      elseif os.get() == "linux" then
         links { "m" }         
      end
      links { "SDL2" }

   project "Spiellib"
      kind "StaticLib"
      language "C"
      files { "src/*.h", "src/*.c", "src/graphics/*.c" }
      
   project "sdltest"
      kind "ConsoleApp"
      links { "Spiellib" }
      language "C"
      files "sdl.c"
      if os.get() == "linux" then
         links { "m" }         
      end
      links { "SDL2" }