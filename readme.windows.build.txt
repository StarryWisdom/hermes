Building instructions for windows

Pre requisites
conan (for getting dependencies)
CMake
MSVC community edition 2019 is what I am currently using, 2017 was used and may still work, C++17 support is required so probably no earlier 


Instructions :-

clone the git repo
In root directory run build-setup.bat (this will initialize conan and download dependencies)

Run cmake
enter the current directory to "where is the source code"
the above with "\build" into "where to build the binaries" (this should of been created by build-setup.bash)
configure, select the compiler, MSVC 2019 with defaults should work, ensure that x64 is the build platform (this should be the default)
press generate
press open project

MSVC should open, F7 will build
