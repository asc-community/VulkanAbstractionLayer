cmake_minimum_required(VERSION 3.12)

project(VulkanAbstractionLayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME, VulkanAbstractionLayer)
set(SOURCES 
"EntryPoint.cpp"
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

"submodules/imgui/imgui.cpp"
"submodules/imgui/imgui_demo.cpp"
"submodules/imgui/imgui_draw.cpp"
"submodules/imgui/imgui_tables.cpp"
"submodules/imgui/imgui_widgets.cpp"
"submodules/imgui/backends/imgui_impl_vulkan.cpp"
"submodules/imgui/backends/imgui_impl_glfw.cpp" "VulkanAbstractionLayer/CommandBuffer.cpp" "VulkanAbstractionLayer/RenderPass.cpp")

find_package(Vulkan REQUIRED FATAL_ERROR)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/glfw)
set(GLFW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glfw/include)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glm)
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui)
set(VMA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/VulkanMemoryAllocator/src)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${Vulkan_INCLUDE_DIR} 
    ${GLFW_INCLUDE_DIR} 
    ${GLM_INCLUDE_DIR} 
    ${IMGUI_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} glfw)

target_compile_definitions(${PROJECT_NAME} PUBLIC -D APPLICATION_WORKING_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")