// Copyright(c) 2021, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanMemoryAllocator.h"

namespace VulkanAbstractionLayer
{
    struct BufferUsageType
    {
        using Value = uint32_t;

        enum Bits : Value
        {
            TRANSFER_SOURCE = (uint32_t)vk::BufferUsageFlagBits::eTransferSrc,
            TRANSFER_DESTINATION = (uint32_t)vk::BufferUsageFlagBits::eTransferDst,
            UNIFORM_TEXEL_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eUniformTexelBuffer,
            STORAGE_TEXEL_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eStorageTexelBuffer,
            UNIFORM_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eUniformBuffer,
            STORAGE_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eStorageBuffer,
            INDEX_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eIndexBuffer,
            VERTEX_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eVertexBuffer,
            INDIRECT_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eIndexBuffer,
            SHADER_DEVICE_ADDRESS = (uint32_t)vk::BufferUsageFlagBits::eShaderDeviceAddress,
            TRANSFORM_FEEDBACK_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT,
            TRANSFORM_FEEDBACK_COUNTER_BUFFER = (uint32_t)vk::BufferUsageFlagBits::eTransformFeedbackCounterBufferEXT,
            CONDITIONAL_RENDERING = (uint32_t)vk::BufferUsageFlagBits::eConditionalRenderingEXT,
            ACCELERATION_STRUCTURE_BUILD_INPUT_READONLY = (uint32_t)vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            ACCELERATION_STRUCTURE_STORAGE = (uint32_t)vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
            SHADER_BINDING_TABLE = (uint32_t)vk::BufferUsageFlagBits::eShaderBindingTableKHR,
        };
    };

    class Buffer
    {
        vk::Buffer handle;
        size_t byteSize = 0;
        VmaAllocation allocation = { };
        uint8_t* mappedMemory = nullptr;

        void Destroy();
    public:
        Buffer() = default;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;
        ~Buffer();

        Buffer(size_t byteSize, BufferUsageType::Value usage, MemoryUsage memoryUsage);
        void Init(size_t byteSize, BufferUsageType::Value usage, MemoryUsage memoryUsage);

        vk::Buffer GetNativeHandle() const { return this->handle; }
        size_t GetByteSize() const { return this->byteSize; }

        bool IsMemoryMapped() const;
        uint8_t* MapMemory();
        void UnmapMemory();
        void FlushMemory();
        void FlushMemory(size_t byteSize, size_t offset);
        void LoadData(const uint8_t* data, size_t byteSize, size_t offset);
    };
}