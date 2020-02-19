:: Mesh
"tools/glslc.exe" -fshader-stage=vertex assets/shaders/vertex.glsl -o assets/shaders/vertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/fragment.glsl -o assets/shaders/fragment.spv
:: Raytracing
"tools/glslc.exe" -fshader-stage=rmiss assets/shaders/raytracing/miss.glsl -o assets/shaders/raytracing/miss.spv 
"tools/glslc.exe" -fshader-stage=rgen assets/shaders/raytracing/raygen.glsl -o assets/shaders/raytracing/raygen.spv
"tools/glslc.exe" -fshader-stage=rchit assets/shaders/raytracing/closesthit.glsl -o assets/shaders/raytracing/closesthit.spv 
:: Particles
"tools/glslc.exe" -fshader-stage=vertex assets/shaders/particles/vertex.glsl -o assets/shaders/particles/vertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/particles/fragment.glsl -o assets/shaders/particles/fragment.spv 
pause
