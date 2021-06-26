# CMake generated Testfile for 
# Source directory: /home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang
# Build directory: /home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(glslang-testsuite "bash" "runtests" "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/localResults" "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/StandAlone/glslangValidator" "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/StandAlone/spirv-remap")
set_tests_properties(glslang-testsuite PROPERTIES  WORKING_DIRECTORY "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/Test/" _BACKTRACE_TRIPLES "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/CMakeLists.txt;351;add_test;/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/CMakeLists.txt;0;")
subdirs("External")
subdirs("glslang")
subdirs("OGLCompilersDLL")
subdirs("StandAlone")
subdirs("SPIRV")
subdirs("hlsl")
subdirs("gtests")
