glslangValidator -V -S vert shadow_vertex.glsl -o main_vertex.spv
glslangValidator -V -S frag shadow_fragment.glsl -o main_fragment.spv
glslangValidator -V -S vert skybox_vertex.glsl -o main_vertex.spv
glslangValidator -V -S frag skybox_fragment.glsl -o main_fragment.spv
glslangValidator -V -S vert main_vertex.glsl -o main_vertex.spv
glslangValidator -V -S frag main_fragment.glsl -o main_fragment.spv