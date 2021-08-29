glslangValidator -V -S vert main_vertex.glsl -o main_vertex.spv
glslangValidator -V -S frag main_fragment.glsl -o main_fragment.spv
glslangValidator -V -S vert reflection_probe_vertex.glsl -o reflection_probe_vertex.spv
glslangValidator -V -S frag reflection_probe_fragment.glsl -o reflection_probe_fragment.spv