@echo off
echo STARTING
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 > vcvars_out.txt 2>&1
echo VCVARS_DONE
echo LIB=%LIB%
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Ninja" > cmake_out.txt 2>&1
echo CMAKE_EXIT=%ERRORLEVEL%
