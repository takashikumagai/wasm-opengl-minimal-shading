emcc -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES3=1 -s "EXPORTED_RUNTIME_METHODS=['ccall']" -s "EXPORTED_FUNCTIONS=[_wasm_init, _wasm_render, _wasm_onresize]" Wasm.cpp MyApp.cpp
