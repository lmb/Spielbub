{ pkgs ? import <nixpkgs> {}, frameworks ? pkgs.darwin.apple_sdk.frameworks }:

pkgs.mkShell {
	buildInputs = with pkgs; [
		# Core build tools
		clang
		gnumake
		premake4

		# Libraries explicitly referenced in premake4.lua
		SDL2
		check # Used in tests project
		frameworks.Cocoa
	];

	# Set clang as the default compiler
	shellHook = ''
		export CC=clang
		export CXX=clang++
		echo "Development environment ready"
		echo "Using clang: $(clang --version | head -n 1)"
	'';

	# Include headers for explicitly referenced libraries
	CPATH = with pkgs; lib.makeSearchPathOutput "dev" "include" [
		SDL2
		check
	];
} 
