set(SOURCES 
"EntryPoint.cpp"
)

add_executable(clothsim ${SOURCES})

target_link_directories(clothsim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(clothsim PUBLIC VulkanAbstractionLayer)

target_include_directories(clothsim PUBLIC ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR})

target_compile_definitions(clothsim PUBLIC -D APPLICATION_WORKING_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")