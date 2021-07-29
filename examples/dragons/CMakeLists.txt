set(SOURCES 
"EntryPoint.cpp"
)

add_executable(Dragons ${SOURCES})

target_link_directories(Dragons PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(Dragons PUBLIC VulkanAbstractionLayer)

target_include_directories(Dragons PUBLIC ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR})

target_compile_definitions(Dragons PUBLIC -D APPLICATION_WORKING_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")