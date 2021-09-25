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

#include <vector>
#include <string>

#include "Buffer.h"
#include "Image.h"
#include "ArrayUtils.h"

namespace VulkanAbstractionLayer
{
	class DependencyStorage
	{
		struct BufferDependencyByValue
		{
            VkBuffer BufferNativeHandle;
            BufferUsage::Bits Usage;
		};

        struct ImageDependencyByValue
        {
            VkImage ImageNativeHandle;
            ImageUsage::Bits Usage;
        };

        struct ImageDependencyByName
        {
            std::string Name;
            ImageUsage::Bits Usage;
        };

        struct BufferDependencyByName
        {
            std::string Name;
            BufferUsage::Bits Usage;
        };

        std::vector<BufferDependencyByValue> bufferDependenciesByValue;
        std::vector<ImageDependencyByValue> imageDependenciesByValue;
        std::vector<BufferDependencyByName> bufferDependenciesByName;
        std::vector<ImageDependencyByName> imageDependenciesByName;
    public:
        void AddBuffer(const Buffer& buffer, BufferUsage::Bits usage);
        void AddBuffer(const std::string& buffer, BufferUsage::Bits usage);
        void AddBuffers(ArrayView<BufferReference> buffers, BufferUsage::Bits usage);
        void AddBuffers(ArrayView<Buffer> buffers, BufferUsage::Bits usage);
        void AddBuffers(ArrayView<std::string> buffers, BufferUsage::Bits usage);
        
        void AddImage(const Image& image, ImageUsage::Bits usage);
        void AddImage(const std::string& buffer, ImageUsage::Bits usage);
        void AddImages(ArrayView<ImageReference> images, ImageUsage::Bits usage);
        void AddImages(ArrayView<Image> images, ImageUsage::Bits usage);
        void AddImages(ArrayView<std::string> images, ImageUsage::Bits usage);

        const auto& GetBufferDependenciesByName() const { return this->bufferDependenciesByName; }
        const auto& GetImageDependenciesByName() const { return this->imageDependenciesByName; }
        const auto& GetBufferDependenciesByValue() const { return this->bufferDependenciesByValue; }
        const auto& GetImageDependenciesByValue() const { return this->imageDependenciesByValue; }
	};
}