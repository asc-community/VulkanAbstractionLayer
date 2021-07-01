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

#include "DescriptorBinding.h"
#include "VulkanContext.h"

namespace VulkanAbstractionLayer
{
	size_t DescriptorBinding::AllocateBinding(const Buffer& buffer)
	{
		this->descriptorBufferInfos.push_back(vk::DescriptorBufferInfo{
			buffer.GetNativeHandle(),
			0,
			buffer.GetByteSize()
		});
		return this->descriptorBufferInfos.size() - 1;
	}

	size_t DescriptorBinding::AllocateBinding(const Image& image)
	{
		this->descriptorImageInfos.push_back(vk::DescriptorImageInfo{
			{ },
			image.GetNativeView(),
			vk::ImageLayout::eShaderReadOnlyOptimal
		});
		return this->descriptorImageInfos.size() - 1;
	}

	size_t DescriptorBinding::AllocateBinding(const Sampler& sampler)
	{
		this->descriptorImageInfos.push_back(vk::DescriptorImageInfo{
			sampler.GetNativeHandle(),
			{ },
			vk::ImageLayout::eShaderReadOnlyOptimal
		});
		return this->descriptorImageInfos.size() - 1;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Buffer& buffer, UniformType type)
	{
		size_t index = this->AllocateBinding(buffer);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Image& image, UniformType type)
	{
		size_t index = this->AllocateBinding(image);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Sampler& sampler, UniformType type)
	{
		size_t index = this->AllocateBinding(sampler);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const std::vector<BufferReference>& buffers, UniformType type)
	{
		size_t index = 0;
		for (const auto& buffer : buffers)
			index = this->AllocateBinding(buffer.get());

		this->descriptorWrites.push_back({ 
			type,
			binding,
			uint32_t(index + 1 - buffers.size()),
			uint32_t(buffers.size())
		});

		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const std::vector<ImageReference>& images, UniformType type)
	{
		size_t index = 0;
		for (const auto& image : images)
			index = this->AllocateBinding(image.get());

		this->descriptorWrites.push_back({
			type,
			binding,
			uint32_t(index + 1 - images.size()),
			uint32_t(images.size())
		});

		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const std::vector<SamplerReference>& samplers, UniformType type)
	{
		size_t index = 0;
		for (const auto& sampler : samplers)
			index = this->AllocateBinding(sampler.get());

		this->descriptorWrites.push_back({
			type,
			binding,
			uint32_t(index + 1 - samplers.size()),
			uint32_t(samplers.size())
		});

		return *this;
	}

	bool IsBufferType(UniformType type)
	{
		switch (type)
		{
		case UniformType::UNIFORM_TEXEL_BUFFER:
		case UniformType::STORAGE_TEXEL_BUFFER:
		case UniformType::UNIFORM_BUFFER:
		case UniformType::STORAGE_BUFFER:
		case UniformType::UNIFORM_BUFFER_DYNAMIC:
		case UniformType::STORAGE_BUFFER_DYNAMIC:
		case UniformType::INLINE_UNIFORM_BLOCK_EXT:
			return true;
		default:
			return false;
		}
	}

	void DescriptorBinding::Write(const vk::DescriptorSet& descriptorSet) const
	{
		std::vector<vk::WriteDescriptorSet> writesDescriptorSet;
		writesDescriptorSet.reserve(this->descriptorWrites.size());

		for (const auto& write : this->descriptorWrites)
		{
			auto& writeDescriptorSet = writesDescriptorSet.emplace_back();
			writeDescriptorSet
				.setDstSet(descriptorSet)
				.setDstBinding(write.Binding)
				.setDescriptorType(ToNative(write.Type))
				.setDescriptorCount(write.Count);

			if(IsBufferType(write.Type))
				writeDescriptorSet.setPBufferInfo(this->descriptorBufferInfos.data() + write.FirstIndex);
			else
				writeDescriptorSet.setPImageInfo(this->descriptorImageInfos.data() + write.FirstIndex);
		}

		GetCurrentVulkanContext().GetDevice().updateDescriptorSets(writesDescriptorSet, { });
	}
}