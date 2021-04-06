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

#include "Buffer.h"
#include "vk_mem_alloc.h"
#include <cassert>

namespace VulkanAbstractionLayer
{
    Buffer::Buffer(size_t byteSize, BufferUsageType::Value usage, MemoryUsage memoryUsage, VmaAllocator allocator)
        : Buffer(allocator)
    {
        this->Init(byteSize, usage, memoryUsage);
    }

    void Buffer::Init(size_t byteSize, BufferUsageType::Value usage, MemoryUsage memoryUsage)
    {
        constexpr std::array BufferQueueFamiliyIndicies = { (uint32_t)0 };
        // destroy previous buffer
        this->Destroy();

        this->byteSize = byteSize;
        vk::BufferCreateInfo bufferCreateInfo;
        bufferCreateInfo
            .setSize(this->byteSize)
            .setUsage((vk::BufferUsageFlags)usage)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setQueueFamilyIndices(BufferQueueFamiliyIndicies);

        VmaAllocationCreateInfo allocationInfo = {};
        allocationInfo.usage = (VmaMemoryUsage)MemoryUsageToNative(memoryUsage);

        (void)vmaCreateBuffer(this->allocator, (VkBufferCreateInfo*)&bufferCreateInfo, &allocationInfo, (VkBuffer*)&this->handle, &this->allocation, nullptr);
    }

    uint8_t* Buffer::MapMemory()
    {
        void* result = nullptr;
        (void)vmaMapMemory(this->allocator, this->allocation, &result);
        return (uint8_t*)result;
    }

    void Buffer::UnmapMemory()
    {
        vmaUnmapMemory(this->allocator, this->allocation);
    }

    void Buffer::FlushMemory()
    {
        this->FlushMemory(this->byteSize, 0);
    }

    void Buffer::FlushMemory(size_t byteSize, size_t offset)
    {
        (void)vmaFlushAllocation(this->allocator, this->allocation, offset, byteSize);
    }

    void Buffer::LoadData(const uint8_t* data, size_t byteSize, size_t offset)
    {
        assert(byteSize + offset <= this->byteSize);
        uint8_t* mappedMemory = this->MapMemory();
        std::memcpy((void*)mappedMemory, (const void*)(data + offset), byteSize);
        this->FlushMemory();
        this->UnmapMemory();
    }

    void Buffer::Destroy()
    {
        if ((bool)this->handle)
        {
            (void)vmaDestroyBuffer(this->allocator, this->handle, this->allocation);
            this->handle = vk::Buffer{ };
        }
    }

    Buffer::Buffer(Buffer&& other) noexcept
    {
        this->handle = other.handle;
        this->byteSize = other.byteSize;
        this->allocator = other.allocator;
        this->allocation = other.allocation;

        other.handle = vk::Buffer{ };
        other.byteSize = 0;
        other.allocation = { };
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        this->Destroy();

        this->handle = other.handle;
        this->allocator = other.allocator;
        this->allocation = other.allocation;

        other.handle = vk::Buffer{ };
        other.byteSize = 0;
        other.allocation = { };
        
        return *this;
    }

    Buffer::~Buffer()
    {
        this->Destroy();
    }
}