solution "Spielbub"
   configurations { "Debug", "Release" }
   includedirs { "/usr/local/include", "include" }
   libdirs { "/usr/local/include" }

   flags { "FatalWarnings" }

   if not os.get() == "windows" then
      buildoptions { "-ansi", "-std=c99", "-pedantic", "-ffunction-sections",
         "-Wextra" }
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
      files { "src/runner/*.h", "src/runner/*.c" }
      links { "Spiellib" }

      links { "SDLmain", "sdl" }

      if os.get() == "macosx" then
         links { "Cocoa.framework" }
      end

   project "Spieltest"
      kind "ConsoleApp"
      language "C"
      files { "src/tests/*.c" }
      links { "Spiellib", "check" }

      links { "SDLmain", "sdl" }

      if os.get() == "macosx" then
         links { "Cocoa.framework" }
      end

   project "Spiellib"
      kind "StaticLib"
      language "C"
      files { "src/*.h", "src/*.c" }