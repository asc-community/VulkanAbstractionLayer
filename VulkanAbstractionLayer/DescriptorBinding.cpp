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
	Sampler EmptySampler;

	ImageUsage::Bits UniformTypeToImageUsage(UniformType type)
	{
		switch (type)
		{
		case UniformType::SAMPLER:
			return ImageUsage::UNKNOWN;
		case UniformType::COMBINED_IMAGE_SAMPLER:
			return ImageUsage::SHADER_READ;
		case UniformType::SAMPLED_IMAGE:
			return ImageUsage::SHADER_READ;
		case UniformType::STORAGE_IMAGE:
			return ImageUsage::STORAGE;
		case UniformType::UNIFORM_TEXEL_BUFFER:
			return ImageUsage::UNKNOWN;
		case UniformType::STORAGE_TEXEL_BUFFER:
			return ImageUsage::UNKNOWN;
		case UniformType::UNIFORM_BUFFER:
			return ImageUsage::UNKNOWN;
		case UniformType::STORAGE_BUFFER:
			return ImageUsage::UNKNOWN;
		case UniformType::UNIFORM_BUFFER_DYNAMIC:
			return ImageUsage::UNKNOWN;
		case UniformType::STORAGE_BUFFER_DYNAMIC:
			return ImageUsage::UNKNOWN;
		case UniformType::INPUT_ATTACHMENT:
			return ImageUsage::INPUT_ATTACHMENT;
		case UniformType::INLINE_UNIFORM_BLOCK_EXT:
			return ImageUsage::UNKNOWN;
		case UniformType::ACCELERATION_STRUCTURE_KHR:
			return ImageUsage::UNKNOWN;
		default:
			return ImageUsage::UNKNOWN;
		}
	}

	BufferUsage::Bits UniformTypeToBufferUsage(UniformType type)
	{
		switch (type)
		{
		case UniformType::SAMPLER:
			return BufferUsage::UNKNOWN;
		case UniformType::COMBINED_IMAGE_SAMPLER:
			return BufferUsage::UNKNOWN;
		case UniformType::SAMPLED_IMAGE:
			return BufferUsage::UNKNOWN;
		case UniformType::STORAGE_IMAGE:
			return BufferUsage::UNKNOWN;
		case UniformType::UNIFORM_TEXEL_BUFFER:
			return BufferUsage::UNIFORM_TEXEL_BUFFER;
		case UniformType::STORAGE_TEXEL_BUFFER:
			return BufferUsage::STORAGE_TEXEL_BUFFER;
		case UniformType::UNIFORM_BUFFER:
			return BufferUsage::UNIFORM_BUFFER;
		case UniformType::STORAGE_BUFFER:
			return BufferUsage::STORAGE_BUFFER;
		case UniformType::UNIFORM_BUFFER_DYNAMIC:
			return BufferUsage::UNIFORM_BUFFER;
		case UniformType::STORAGE_BUFFER_DYNAMIC:
			return BufferUsage::STORAGE_BUFFER;
		case UniformType::INPUT_ATTACHMENT:
			return BufferUsage::UNKNOWN;
		case UniformType::INLINE_UNIFORM_BLOCK_EXT:
			return BufferUsage::UNIFORM_BUFFER;
		case UniformType::ACCELERATION_STRUCTURE_KHR:
			return BufferUsage::ACCELERATION_STRUCTURE_STORAGE;
		default:
			return BufferUsage::UNKNOWN;
		}
	}

	size_t DescriptorBinding::AllocateBinding(const Buffer& buffer, UniformType type)
	{
		this->bufferWriteInfos.push_back(BufferWriteInfo{
			std::addressof(buffer),
			UniformTypeToBufferUsage(type),
		});
		return this->bufferWriteInfos.size() - 1;
	}

	size_t DescriptorBinding::AllocateBinding(const Image& image, ImageView view, UniformType type)
	{
		this->imageWriteInfos.push_back(ImageWriteInfo{
			std::addressof(image),
			UniformTypeToImageUsage(type),
			view,
			{ },
		});
		return this->imageWriteInfos.size() - 1;
	}

	size_t DescriptorBinding::AllocateBinding(const Image& image, const Sampler& sampler, ImageView view, UniformType type)
	{
		this->imageWriteInfos.push_back(ImageWriteInfo{
			std::addressof(image),
			UniformTypeToImageUsage(type),
			view,
			std::addressof(sampler),
		});
		return this->imageWriteInfos.size() - 1;
	}

	size_t DescriptorBinding::AllocateBinding(const Sampler& sampler)
	{
		this->imageWriteInfos.push_back(ImageWriteInfo{
			{ },
			ImageUsage::UNKNOWN,
			{ },
			std::addressof(sampler),
		});
		return this->imageWriteInfos.size() - 1;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Buffer& buffer, UniformType type)
	{
		size_t index = this->AllocateBinding(buffer, type);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Image& image, UniformType type)
	{
		this->Bind(binding, image, type, ImageView::NATIVE);
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Sampler& sampler, UniformType type)
	{
		size_t index = this->AllocateBinding(sampler);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Image& image, const Sampler& sampler, UniformType type)
	{
		size_t index = this->AllocateBinding(image, sampler, ImageView::NATIVE, type);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Image& image, UniformType type, ImageView view)
	{
		size_t index = this->AllocateBinding(image, view, type);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, const Image& image, const Sampler& sampler, UniformType type, ImageView view)
	{
		size_t index = this->AllocateBinding(image, sampler, view, type);
		this->descriptorWrites.push_back({ type, binding, (uint16_t)index, 1, });
		return *this;
	}
	
	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, StringId attachment, UniformType type, ImageView view)
	{
		return this->Bind(binding, attachment, EmptySampler, type, view);
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, StringId attachment, const Sampler& sampler, UniformType type, ImageView view)
	{
		this->attachmentWriteInfos.push_back(AttachmentResolveInfo{
			attachment,
			binding,
			type,
			UniformTypeToImageUsage(type),
			view,
			sampler
		});
		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, ArrayView<BufferReference> buffers, UniformType type)
	{
		size_t index = 0;
		for (const auto& buffer : buffers)
			index = this->AllocateBinding(buffer.get(), type);

		this->descriptorWrites.push_back({ 
			type,
			binding,
			uint32_t(index + 1 - buffers.size()),
			uint32_t(buffers.size())
		});

		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, ArrayView<Buffer> buffers, UniformType type)
	{
		size_t index = 0;
		for (const auto& buffer : buffers)
			index = this->AllocateBinding(buffer, type);

		this->descriptorWrites.push_back({ 
			type,
			binding,
			uint32_t(index + 1 - buffers.size()),
			uint32_t(buffers.size())
		});

		return *this;
	}

	void DescriptorBinding::ResolveAttachments(const std::unordered_map<StringId, Image>& mappings)
	{
		for (const auto& attachmentInfo : this->attachmentWriteInfos)
		{
			this->Bind(attachmentInfo.Binding, mappings.at(attachmentInfo.Name), attachmentInfo.SamplerHandle.get(), attachmentInfo.Type, attachmentInfo.View);
		}
		this->attachmentWriteInfos.clear();
	}

	void DescriptorBinding::ResolveAttachments(const std::unordered_map<StringId, ImageReference>& mappings)
	{
		for (const auto& attachmentInfo : this->attachmentWriteInfos)
		{
			this->Bind(attachmentInfo.Binding, mappings.at(attachmentInfo.Name).get(), attachmentInfo.SamplerHandle.get(), attachmentInfo.Type, attachmentInfo.View);
		}
		this->attachmentWriteInfos.clear();
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, ArrayView<ImageReference> images, UniformType type)
	{
		size_t index = 0;
		for (const auto& image : images)
			index = this->AllocateBinding(image.get(), ImageView::NATIVE, type);

		this->descriptorWrites.push_back({
			type,
			binding,
			uint32_t(index + 1 - images.size()),
			uint32_t(images.size())
		});

		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, ArrayView<Image> images, UniformType type)
	{
		size_t index = 0;
		for (const auto& image : images)
			index = this->AllocateBinding(image, ImageView::NATIVE, type);

		this->descriptorWrites.push_back({
			type,
			binding,
			uint32_t(index + 1 - images.size()),
			uint32_t(images.size())
		});

		return *this;
	}

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, ArrayView<SamplerReference> samplers, UniformType type)
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

	DescriptorBinding& DescriptorBinding::Bind(uint32_t binding, ArrayView<Sampler> samplers, UniformType type)
	{
		size_t index = 0;
		for (const auto& sampler : samplers)
			index = this->AllocateBinding(sampler);

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
		std::vector<vk::DescriptorBufferInfo> descriptorBufferInfos;
		std::vector<vk::DescriptorImageInfo> descriptorImageInfos;
		writesDescriptorSet.reserve(this->descriptorWrites.size());
		descriptorBufferInfos.reserve(this->bufferWriteInfos.size());
		descriptorImageInfos.reserve(this->imageWriteInfos.size());

		for (const auto& bufferInfo : this->bufferWriteInfos)
		{
			descriptorBufferInfos.push_back(vk::DescriptorBufferInfo{
				bufferInfo.Handle->GetNativeHandle(),
				0,
				bufferInfo.Handle->GetByteSize(),
			});
		}

		for (const auto& imageInfo : this->imageWriteInfos)
		{
			descriptorImageInfos.push_back(vk::DescriptorImageInfo{
				imageInfo.SamplerHandle != nullptr ? imageInfo.SamplerHandle->GetNativeHandle() : nullptr,
				imageInfo.Handle != nullptr ? imageInfo.Handle->GetNativeView(imageInfo.View) : nullptr,
				ImageUsageToImageLayout(imageInfo.Usage),
			});
		}

		for (const auto& write : this->descriptorWrites)
		{
			auto& writeDescriptorSet = writesDescriptorSet.emplace_back();
			writeDescriptorSet
				.setDstSet(descriptorSet)
				.setDstBinding(write.Binding)
				.setDescriptorType(ToNative(write.Type))
				.setDescriptorCount(write.Count);

			if (IsBufferType(write.Type))
			{
				writeDescriptorSet.setPBufferInfo(descriptorBufferInfos.data() + write.FirstIndex);
			}
			else
			{
				writeDescriptorSet.setPImageInfo(descriptorImageInfos.data() + write.FirstIndex);
			}
		}

		GetCurrentVulkanContext().GetDevice().updateDescriptorSets(writesDescriptorSet, { });
	}
}