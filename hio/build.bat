cd /d %~dp0

rd /s /q _build

mkdir _build
cd _build

cmake .. -G "Visual Studio 16 2019" -DHFS="C:/Program Files/Side Effects Software/Houdini 18.0.566"
cmake --build . --config release
del CMakeCache.txt

cd ..
rd /s /q _build
