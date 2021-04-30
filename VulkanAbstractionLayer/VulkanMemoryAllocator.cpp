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

#define VMA_IMPLEMENTATION

#include "VulkanMemoryAllocator.h"
#include "VulkanContext.h"
#include "vk_mem_alloc.h"

namespace VulkanAbstractionLayer
{
    VmaMemoryUsage MemoryUsageToNative(MemoryUsage usage)
    {
        constexpr VmaMemoryUsage mappingTable[] = {
            VMA_MEMORY_USAGE_GPU_ONLY,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_MEMORY_USAGE_GPU_TO_CPU,
            VMA_MEMORY_USAGE_CPU_COPY,
            VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED,
        };
        return mappingTable[(size_t)usage];
    }

    VmaAllocator GetVulkanAllocator()
    {
        return GetCurrentVulkanContext().GetAllocator();
    }

    void DeallocateImage(const vk::Image& image, VmaAllocation allocation)
    {
        vmaDestroyImage(GetVulkanAllocator(), image, allocation);
    }

    void DeallocateBuffer(const vk::Buffer& buffer, VmaAllocation allocation)
    {
        vmaDestroyBuffer(GetVulkanAllocator(), buffer, allocation);
    }

    VmaAllocation AllocateImage(const vk::ImageCreateInfo& imageCreateInfo, MemoryUsage usage, vk::Image* image)
    {
        VmaAllocation allocation = { };
        VmaAllocationCreateInfo allocationInfo = { };
        allocationInfo.usage = MemoryUsageToNative(usage);
        (void)vmaCreateImage(GetCurrentVulkanContext().GetAllocator(), (VkImageCreateInfo*)&imageCreateInfo, &allocationInfo, (VkImage*)image, &allocation, nullptr);
        return allocation;
    }

    VmaAllocation AllocateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, MemoryUsage usage, vk::Buffer* buffer)
    {
        VmaAllocation allocation = { };
        VmaAllocationCreateInfo allocationInfo = { };
        allocationInfo.usage = MemoryUsageToNative(usage);
        (void)vmaCreateBuffer(GetCurrentVulkanContext().GetAllocator(), (VkBufferCreateInfo*)&bufferCreateInfo, &allocationInfo, (VkBuffer*)buffer, &allocation, nullptr);
        return allocation;
    }

    uint8_t* MapMemory(VmaAllocation allocation)
    {
        void* memory = nullptr;
        vmaMapMemory(GetCurrentVulkanContext().GetAllocator(), allocation, &memory);
        return (uint8_t*)memory;
    }

    void UnmapMemory(VmaAllocation allocation)
    {
        vmaUnmapMemory(GetCurrentVulkanContext().GetAllocator(), allocation);
    }

    void FlushMemory(VmaAllocation allocation, size_t byteSize, size_t offset)
    {
        vmaFlushAllocation(GetCurrentVulkanContext().GetAllocator(), allocation, offset, byteSize);
    }
}
