glslangValidator -V -S vert main_vertex.glsl -o main_vertex.spv
glslangValidator -V -S frag main_fragment.glsl -o main_fragment.spv
glslangValidator -V -S comp main_compute.glsl -o main_compute.spv