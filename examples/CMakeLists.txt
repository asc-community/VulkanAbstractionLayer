set(SOURCES 
"EntryPoint.cpp"
)

add_executable(VulkanSample ${SOURCES})

target_link_directories(VulkanSample PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(VulkanSample PUBLIC VulkanAbstractionLayer)

target_include_directories(VulkanSample PUBLIC ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR})

target_compile_definitions(VulkanSample PUBLIC -D APPLICATION_WORKING_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")