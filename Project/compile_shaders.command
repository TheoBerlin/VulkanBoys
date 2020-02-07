DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"
./tools/glslc -fshader-stage=vertex assets/shaders/vertex.glsl -o assets/shaders/vertex.spv
./tools/glslc -fshader-stage=fragment assets//shaders/fragment.glsl -o assets/shaders/fragment.spv