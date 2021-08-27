#include "Pipeline.h"
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

#include "Pipeline.h"

namespace VulkanAbstractionLayer
{
	void Pipeline::DeclareBuffer(const Buffer& buffer)
	{
		this->bufferDeclarations.push_back(BufferDeclaration{ 
			(VkBuffer)buffer.GetNativeHandle(), 
			BufferUsage::UNKNOWN 
		});
	}

	void Pipeline::DeclareImage(const Image& image, ImageUsage::Bits oldUsage)
	{
		this->imageDeclarations.push_back(ImageDeclaration{ 
			(VkImage)image.GetNativeHandle(), 
			oldUsage, 
			image.GetFormat(), 
			image.GetMipLevelCount(),
			image.GetLayerCount()
		});
	}

	void Pipeline::DeclareBuffers(ArrayView<BufferReference> buffers)
	{
		this->bufferDeclarations.reserve(this->bufferDeclarations.size() + buffers.size());
		for (const auto& bufferReference : buffers)
			this->DeclareBuffer(bufferReference.get());
	}

	void Pipeline::DeclareImages(ArrayView<ImageReference> images, ImageUsage::Bits oldUsage)
	{
		this->imageDeclarations.reserve(this->imageDeclarations.size() + images.size());
		for (const auto& imageReference : images)
			this->DeclareImage(imageReference.get(), oldUsage);
	}

	void Pipeline::DeclareBuffers(ArrayView<Buffer> buffers)
	{
		this->bufferDeclarations.reserve(this->bufferDeclarations.size() + buffers.size());
		for (const auto& buffer : buffers)
			this->DeclareBuffer(buffer);
	}

	void Pipeline::DeclareImages(ArrayView<Image> images, ImageUsage::Bits oldUsage)
	{
		this->imageDeclarations.reserve(this->imageDeclarations.size() + images.size());
		for (const auto& image : images)
			this->DeclareImage(image, oldUsage);
	}

	void Pipeline::DeclareAttachment(StringId name, Format format)
	{
		this->attachmentDeclarations.push_back(AttachmentDeclaration{ 
			name, 
			format, 
			0, 0 
		});
	}

	void Pipeline::DeclareAttachment(StringId name, Format format, uint32_t width, uint32_t height)
	{
		this->attachmentDeclarations.push_back(AttachmentDeclaration{ 
			name, 
			format, 
			width, height 
		});
	}

	void Pipeline::AddOutputAttachment(StringId name, ClearColor clear)
	{
		this->outputAttachments.push_back(OutputAttachment{ 
			name, 
			clear, 
			ClearDepthStencil{ }, 
			AttachmentState::CLEAR_COLOR 
		});
	}

	void Pipeline::AddOutputAttachment(StringId name, ClearDepthStencil clear)
	{
		this->outputAttachments.push_back(OutputAttachment{ 
			name, 
			ClearColor{ },
			clear, 
			AttachmentState::CLEAR_DEPTH_SPENCIL 
		});
	}

	void Pipeline::AddOutputAttachment(StringId name, AttachmentState onLoad)
	{
		this->outputAttachments.push_back(OutputAttachment{ 
			name, 
			ClearColor{ }, 
			ClearDepthStencil{ }, 
			onLoad 
		});
	}
}