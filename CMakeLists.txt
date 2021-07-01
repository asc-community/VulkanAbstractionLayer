cmake_minimum_required(VERSION 3.12)

project(VulkanAbstractionLayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VULKAN_ABSTRACTION_LAYER_BUILD_EXAMPLES "build examples" ON)

set(SOURCES 
"VulkanAbstractionLayer/Window.cpp"
"VulkanAbstractionLayer/VulkanContext.cpp" 
"VulkanAbstractionLayer/VulkanSurface.cpp" 
"VulkanAbstractionLayer/VirtualFrame.cpp"
"VulkanAbstractionLayer/Buffer.cpp" 
"VulkanAbstractionLayer/VulkanMemoryAllocator.cpp" 
"VulkanAbstractionLayer/Image.cpp"
"VulkanAbstractionLayer/RenderGraphBuilder.cpp"
"VulkanAbstractionLayer/RenderGraph.cpp"
"VulkanAbstractionLayer/ImGuiContext.cpp"
"VulkanAbstractionLayer/CommandBuffer.cpp" 
"VulkanAbstractionLayer/RenderPass.cpp" 
"VulkanAbstractionLayer/ShaderLoader.cpp" 
"VulkanAbstractionLayer/GraphicShader.cpp" 
"VulkanAbstractionLayer/ShaderReflection.cpp"
"VulkanAbstractionLayer/ModelLoader.cpp"
"VulkanAbstractionLayer/ImageLoader.cpp"
"submodules/imgui/imgui.cpp"
"submodules/imgui/imgui_demo.cpp"
"submodules/imgui/imgui_draw.cpp"
"submodules/imgui/imgui_tables.cpp"
"submodules/imgui/imgui_widgets.cpp"
"submodules/imgui/backends/imgui_impl_vulkan.cpp"
"submodules/imgui/backends/imgui_impl_glfw.cpp"
"submodules/spirv-reflect/spirv_reflect.c" "VulkanAbstractionLayer/DescriptorCache.h" "VulkanAbstractionLayer/DescriptorCache.cpp" "VulkanAbstractionLayer/Sampler.cpp" "VulkanAbstractionLayer/DescriptorBinding.cpp" "VulkanAbstractionLayer/StageBuffer.cpp" "VulkanAbstractionLayer/DependencyStorage.cpp" "VulkanAbstractionLayer/Pipeline.cpp")

find_package(Vulkan REQUIRED FATAL_ERROR)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/glslang)
set(GLFW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glfw/include)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glm)
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui)
set(GLSLANG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glslang/glslang/Public ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glslang/SPIRV)
set(VMA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/VulkanMemoryAllocator/src)
set(TINYOBJLOADER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/tinyobjloader)
set(SPAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/span/include)
set(SPIRV_REFLECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/spirv-reflect)
set(STB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/stb)

add_library(${CMAKE_PROJECT_NAME} STATIC ${SOURCES})

set(VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIR} 
    ${GLFW_INCLUDE_DIR} 
    ${GLM_INCLUDE_DIR} 
    ${IMGUI_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
    ${GLSLANG_INCLUDE_DIR}
    ${TINYOBJLOADER_INCLUDE_DIR}
    ${SPAN_INCLUDE_DIR}
    ${SPIRV_REFLECT_INCLUDE_DIR}
    ${STB_INCLUDE_DIR}
)

set(VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR} PARENT_SCOPE)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} glfw MachineIndependent SPIRV)

# examples
if(VULKAN_ABSTRACTION_LAYER_BUILD_EXAMPLES)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()