"tools/glslc.exe" -fshader-stage=vertex assets/shaders/vertex.glsl -o assets/shaders/vertex.spv 
"tools/glslc.exe" -fshader-stage=fragment assets/shaders/fragment.glsl -o assets/shaders/fragment.spv
"tools/glslc.exe" -fshader-stage=rgen assets/shaders/raytracing/raygen.glsl -o assets/shaders/raytracing/raygen.spv
"tools/glslc.exe" -fshader-stage=rmiss assets/shaders/raytracing/miss.glsl -o assets/shaders/raytracing/miss.spv 
"tools/glslc.exe" -fshader-stage=rchit assets/shaders/raytracing/closesthit.glsl -o assets/shaders/raytracing/closesthit.spv 
"tools/glslc.exe" -fshader-stage=rmiss assets/shaders/raytracing/missShadow.glsl -o assets/shaders/raytracing/missShadow.spv 
"tools/glslc.exe" -fshader-stage=rchit assets/shaders/raytracing/closesthitShadow.glsl -o assets/shaders/raytracing/closesthitShadow.spv 

"tools/glslc.exe" -fshader-stage=rgen  assets/shaders/raytracing/lightprobes/raygenLPGlossy.glsl 		-o assets/shaders/raytracing/lightprobes/raygenLPGlossy.spv
"tools/glslc.exe" -fshader-stage=rmiss assets/shaders/raytracing/lightprobes/missLPGlossy.glsl 		-o assets/shaders/raytracing/lightprobes/missLPGlossy.spv 
"tools/glslc.exe" -fshader-stage=rchit assets/shaders/raytracing/lightprobes/closesthitLPGlossy.glsl 	-o assets/shaders/raytracing/lightprobes/closesthitLPGlossy.spv 

"tools/glslc.exe" -fshader-stage=rmiss assets/shaders/raytracing/lightprobes/missShadow.glsl 			-o assets/shaders/raytracing/lightprobes/missShadow.spv 
"tools/glslc.exe" -fshader-stage=rchit assets/shaders/raytracing/lightprobes/closesthitShadow.glsl 		-o assets/shaders/raytracing/lightprobes/closesthitShadow.spv 

"tools/glslc.exe" -fshader-stage=rgen  assets/shaders/raytracing/lightprobes/raygen.glsl 				-o assets/shaders/raytracing/lightprobes/raygen.spv
"tools/glslc.exe" -fshader-stage=rmiss assets/shaders/raytracing/lightprobes/miss.glsl 					-o assets/shaders/raytracing/lightprobes/miss.spv 
"tools/glslc.exe" -fshader-stage=rchit assets/shaders/raytracing/lightprobes/closesthit.glsl 			-o assets/shaders/raytracing/lightprobes/closesthit.spv 

"tools/glslc.exe" -fshader-stage=rgen  assets/shaders/raytracing/lightprobes/raygenVisualizer.glsl 				-o assets/shaders/raytracing/lightprobes/raygenVisualizer.spv
"tools/glslc.exe" -fshader-stage=rmiss assets/shaders/raytracing/lightprobes/missVisualizer.glsl 				-o assets/shaders/raytracing/lightprobes/missVisualizer.spv 
"tools/glslc.exe" -fshader-stage=rchit assets/shaders/raytracing/lightprobes/closesthitVisualizer.glsl 			-o assets/shaders/raytracing/lightprobes/closesthitVisualizer.spv 

"tools/glslc.exe" -fshader-stage=compute assets/shaders/raytracing/lightprobes/collapseGI.glsl 			-o assets/shaders/raytracing/lightprobes/collapseGI.spv 
pause