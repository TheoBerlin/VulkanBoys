"tools/glslc.exe" -fshader-stage=vertex assets/shaders/vertex.glsl -o assets/shaders/vertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/fragment.glsl -o assets/shaders/fragment.spv
"tools/glslc.exe" -fshader-stage=vertex assets/shaders/geometryVertex.glsl -o assets/shaders/geometryVertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/geometryFragment.glsl -o assets/shaders/geometryFragment.spv
"tools/glslc.exe" -fshader-stage=vertex assets/shaders/lightVertex.glsl -o assets/shaders/lightVertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/lightFragment.glsl -o assets/shaders/lightFragment.spv
"tools/glslc.exe" -fshader-stage=vertex assets/shaders/panoramaCubemapVertex.glsl -o assets/shaders/panoramaCubemapVertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/panoramaCubemapFragment.glsl -o assets/shaders/panoramaCubemapFragment.spv
"tools/glslc.exe" -fshader-stage=vertex assets/shaders/skyboxVertex.glsl -o assets/shaders/skyboxVertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/skyboxFragment.glsl -o assets/shaders/skyboxFragment.spv
pause