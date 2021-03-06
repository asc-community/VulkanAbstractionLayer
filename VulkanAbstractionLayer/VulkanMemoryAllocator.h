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

#include <cstdint>
#include <cstddef>

struct VmaAllocator_T;
struct VmaAllocation_T;
using VmaAllocator = VmaAllocator_T*;
using VmaAllocation = VmaAllocation_T*;

namespace vk
{
    class Image;
    class Buffer;
    struct ImageCreateInfo;
    struct BufferCreateInfo;
}

namespace VulkanAbstractionLayer
{
    class VulkanContext;

    enum class MemoryUsage
    {
        GPU_ONLY = 0, // device local for fast GPU access
        CPU_ONLY, // heap allocated for staging resources
        CPU_TO_GPU, // dynamic resources with frequent update from CPU
        GPU_TO_CPU, // readback from GPU to CPU
        CPU_COPY, // cpu memory used to cache GPU resources in heap
        GPU_LAZILY_ALLOCATED, // used only on mobile platforms
    };

    VmaAllocator GetVulkanAllocator();
    void DeallocateImage(const vk::Image& image, VmaAllocation allocation);
    void DeallocateBuffer(const vk::Buffer& buffer, VmaAllocation allocation);
    VmaAllocation AllocateImage(const vk::ImageCreateInfo& imageCreateInfo, MemoryUsage usage, vk::Image* image);
    VmaAllocation AllocateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, MemoryUsage usage, vk::Buffer* buffer);
    uint8_t* MapMemory(VmaAllocation allocation);
    void UnmapMemory(VmaAllocation allocation);
    void FlushMemory(VmaAllocation allocation, size_t byteSize, size_t offset);
}