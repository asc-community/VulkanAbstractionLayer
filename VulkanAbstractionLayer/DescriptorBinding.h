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
#include <unordered_map>

#include "Buffer.h"
#include "Image.h"
#include "Sampler.h"
#include "ShaderReflection.h"
#include "ArrayUtils.h"

namespace VulkanAbstractionLayer
{
	class ResolveInfo
	{
		std::unordered_map<std::string, std::vector<BufferReference>> bufferResolves;
		std::unordered_map<std::string, std::vector<ImageReference>> imageResolves;

	public:
		void Resolve(const std::string& name, const Buffer& buffer);
		void Resolve(const std::string& name, ArrayView<const Buffer> buffers);
		void Resolve(const std::string& name, ArrayView<const BufferReference> buffers);

		void Resolve(const std::string& name, const Image& image);
		void Resolve(const std::string& name, ArrayView<const Image> images);
		void Resolve(const std::string& name, ArrayView<const ImageReference> images);

		const auto& GetBuffers() const { return this->bufferResolves; }
		const auto& GetImages() const { return this->imageResolves; }
	};

	enum class ResolveOptions
	{
		RESOLVE_EACH_FRAME,
		RESOLVE_ONCE,
		ALREADY_RESOLVED,
	};

	class DescriptorBinding
	{
		struct DescriptorWriteInfo
		{
			UniformType Type;
			uint32_t Binding;
			uint32_t FirstIndex;
			uint32_t Count;
		};

		struct BufferWriteInfo
		{
			const Buffer* Handle;
			BufferUsage::Bits Usage;
		};

		struct ImageWriteInfo
		{
			const Image* Handle;
			ImageUsage::Bits Usage;
			ImageView View;
			const Sampler* SamplerHandle;
		};

		struct ImageToResolve
		{
			std::string Name;
			uint32_t Binding;
			UniformType Type;
			ImageUsage::Bits Usage;
			ImageView View;
			const Sampler* SamplerHandle;
		};

		struct BufferToResolve
		{
			std::string Name;
			uint32_t Binding;
			UniformType Type;
			BufferUsage::Bits Usage;
		};

		struct SamplerToResolve
		{
			const Sampler* SamplerHandle;
			uint32_t Binding;
			UniformType Type;
		};

		std::vector<DescriptorWriteInfo> descriptorWrites;
		std::vector<BufferWriteInfo> bufferWriteInfos;
		std::vector<ImageWriteInfo> imageWriteInfos;
		std::vector<BufferToResolve> buffersToResolve;
		std::vector<ImageToResolve> imagesToResolve;
		std::vector<SamplerToResolve> samplersToResolve;

		ResolveOptions options = ResolveOptions::RESOLVE_EACH_FRAME;

		size_t AllocateBinding(const Buffer& buffer, UniformType type);
		size_t AllocateBinding(const Image& image, ImageView view, UniformType type);
		size_t AllocateBinding(const Image& image, const Sampler& sampler, ImageView view, UniformType type);
		size_t AllocateBinding(const Sampler& sampler);
	public:
		DescriptorBinding& Bind(uint32_t binding, const std::string& name, UniformType type);
		DescriptorBinding& Bind(uint32_t binding, const std::string& name, UniformType type, ImageView view);
		DescriptorBinding& Bind(uint32_t binding, const std::string& name, const Sampler& sampler, UniformType type);
		DescriptorBinding& Bind(uint32_t binding, const std::string& name, const Sampler& sampler, UniformType type, ImageView view);

		DescriptorBinding& Bind(uint32_t binding, const Sampler& sampler, UniformType type);

		void SetOptions(ResolveOptions options) { this->options = options; }

		void Resolve(const ResolveInfo& resolveInfo);

		void Write(const vk::DescriptorSet& descriptorSet);
		const auto& GetBoundBuffers() const { return this->buffersToResolve; }
		const auto& GetBoundImages() const { return this->imagesToResolve; }
	};
}