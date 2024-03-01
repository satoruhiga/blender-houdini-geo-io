rem build.bat "C:\Program Files\Side Effects Software\Houdini 20.0.625" "C:\Users\satoruhiga\.pyenv\pyenv-win\versions\3.10.11\python.exe"

cd /d %~dp0

rd /s /q _build

mkdir _build
cd _build

cmake .. -G "Visual Studio 17 2022" -DHFS=%1 -DPYTHON_EXECUTABLE=%2
cmake --build . --config release
del CMakeCache.txt

cd ..
rd /s /q _build
