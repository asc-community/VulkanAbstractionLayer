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

#include "DependencyStorage.h"

namespace VulkanAbstractionLayer
{
	void DependencyStorage::AddBuffer(const Buffer& buffer, BufferUsage::Bits usage)
	{
		this->bufferDependenciesByValue.push_back({ (VkBuffer)buffer.GetNativeHandle(), usage });
	}

	void DependencyStorage::AddBuffer(const std::string& buffer, BufferUsage::Bits usage)
	{
		this->bufferDependenciesByName.push_back({ buffer, usage });
	}

	void DependencyStorage::AddBuffers(ArrayView<BufferReference> buffers, BufferUsage::Bits usage)
	{
		for (const auto& buffer : buffers)
			this->AddBuffer(buffer.get(), usage);
	}

	void DependencyStorage::AddBuffers(ArrayView<std::string> buffers, BufferUsage::Bits usage)
	{
		for (const auto& buffer : buffers)
			this->AddBuffer(buffer, usage);
	}

	void DependencyStorage::AddBuffers(ArrayView<Buffer> buffers, BufferUsage::Bits usage)
	{
		for (const auto& buffer : buffers)
			this->AddBuffer(buffer, usage);
	}

	void DependencyStorage::AddImage(const Image& image, ImageUsage::Bits usage)
	{
		this->imageDependenciesByValue.push_back({ (VkImage)image.GetNativeHandle(), usage });
	}

	void DependencyStorage::AddImage(const std::string& image, ImageUsage::Bits usage)
	{
		this->imageDependenciesByName.push_back({ image, usage });
	}

	void DependencyStorage::AddImages(ArrayView<ImageReference> images, ImageUsage::Bits usage)
	{
		for (const auto& image : images)
			this->AddImage(image.get(), usage);
	}

	void DependencyStorage::AddImages(ArrayView<Image> images, ImageUsage::Bits usage)
	{
		for (const auto& image : images)
			this->AddImage(image, usage);
	}

	void DependencyStorage::AddImages(ArrayView<std::string> images, ImageUsage::Bits usage)
	{
		for (const auto& image : images)
			this->AddImage(image, usage);
	}
}