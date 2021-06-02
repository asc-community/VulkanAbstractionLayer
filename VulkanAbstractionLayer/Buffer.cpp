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
#include "VulkanContext.h"
#include <cassert>

namespace VulkanAbstractionLayer
{
    Buffer::Buffer(size_t byteSize, BufferUsageType::Value usage, MemoryUsage memoryUsage)
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

        this->allocation = AllocateBuffer(bufferCreateInfo, memoryUsage, &this->handle);
    }

    bool Buffer::IsMemoryMapped() const
    {
        return this->mappedMemory != nullptr;
    }

    uint8_t* Buffer::MapMemory()
    {
        if(this->mappedMemory == nullptr)
            this->mappedMemory = VulkanAbstractionLayer::MapMemory(this->allocation);
        return this->mappedMemory;
    }

    void Buffer::UnmapMemory()
    {
        VulkanAbstractionLayer::UnmapMemory(this->allocation);
        this->mappedMemory = nullptr;
    }

    void Buffer::FlushMemory()
    {
        this->FlushMemory(this->byteSize, 0);
    }

    void Buffer::FlushMemory(size_t byteSize, size_t offset)
    {
        VulkanAbstractionLayer::FlushMemory(this->allocation, byteSize, offset);
    }

    void Buffer::LoadData(const uint8_t* data, size_t byteSize, size_t offset)
    {
        assert(byteSize + offset <= this->byteSize);

        if (this->mappedMemory == nullptr)
        {
            (void)this->MapMemory();
            std::memcpy((void*)(this->mappedMemory + offset), (const void*)data, byteSize);
            this->FlushMemory(byteSize, offset);
            this->UnmapMemory();
        }
        else // do not do map-unmap if memory was already mapped externally
        {
            std::memcpy((void*)(this->mappedMemory + offset), (const void*)data, byteSize);
            this->FlushMemory(byteSize, offset);
        }
    }

    void Buffer::Destroy()
    {
        if ((bool)this->handle)
        {
            if (this->mappedMemory != nullptr)
                this->UnmapMemory();
            DeallocateBuffer(this->handle, this->allocation);
            this->handle = vk::Buffer{ };
        }
    }

    Buffer::Buffer(Buffer&& other) noexcept
    {
        this->handle = other.handle;
        this->byteSize = other.byteSize;
        this->allocation = other.allocation;
        this->mappedMemory = other.mappedMemory;

        other.handle = vk::Buffer{ };
        other.byteSize = 0;
        other.allocation = { };
        other.mappedMemory = nullptr;
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        this->Destroy();

        this->handle = other.handle;
        this->allocation = other.allocation;
        this->mappedMemory = other.mappedMemory;

        other.handle = vk::Buffer{ };
        other.byteSize = 0;
        other.allocation = { };
        other.mappedMemory = nullptr;
        
        return *this;
    }

    Buffer::~Buffer()
    {
        this->Destroy();
    }
}