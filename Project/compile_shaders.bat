"tools/glslc.exe" -fshader-stage=vertex assets/shaders/vertex.glsl -o assets/shaders/vertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/fragment.glsl -o assets/shaders/fragment.spv

"tools/glslc.exe" -fshader-stage=vertex assets/shaders/geometryVertex.glsl -o assets/shaders/geometryVertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/geometryFragment.glsl -o assets/shaders/geometryFragment.spv

"tools/glslc.exe" -fshader-stage=fragment assets/shaders/lightFragment.glsl -o assets/shaders/lightFragment.spv

"tools/glslc.exe" -fshader-stage=vertex assets/shaders/skyboxVertex.glsl -o assets/shaders/skyboxVertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/skyboxFragment.glsl -o assets/shaders/skyboxFragment.spv

"tools/glslc.exe" -fshader-stage=vertex assets/shaders/fullscreenVertex.glsl -o assets/shaders/fullscreenVertex.spv 

"tools/glslc.exe" -fshader-stage=vertex assets/shaders/filterCubemap.glsl -o assets/shaders/filterCubemap.spv 

"tools/glslc.exe" -fshader-stage=fragment assets/shaders/genCubemapFragment.glsl -o assets/shaders/genCubemapFragment.spv
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/genIntegrationMap.glsl -o assets/shaders/genIntegrationMap.spv
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/genIrradianceFragment.glsl -o assets/shaders/genIrradianceFragment.spv
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/preFilterEnvironment.glsl -o assets/shaders/preFilterEnvironment.spv
pause