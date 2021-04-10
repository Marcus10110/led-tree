
pushd ..\..\..\emsdk\
::emsdk activate latest
call emsdk_env.bat
popd
call em++ --bind -std=c++17 led.cpp animations.cpp -o led.js -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'addOnPostRun']"
copy /Y led.js ..
copy /Y led.wasm ..