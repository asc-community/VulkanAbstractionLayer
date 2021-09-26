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
	void Pipeline::AddDependency(const std::string& name, BufferUsage::Bits usage)
	{
		this->bufferDependencies.push_back(
			BufferDependency{ name, usage }
		);
	}

	void Pipeline::AddDependency(const std::string& name, ImageUsage::Bits usage)
	{
		this->imageDependencies.push_back(
			ImageDependency{ name, usage }
		);
	}

	void Pipeline::DeclareAttachment(const std::string& name, Format format)
	{
		this->DeclareAttachment(name, format, 0, 0, ImageOptions::DEFAULT);
	}

	void Pipeline::DeclareAttachment(const std::string& name, Format format, uint32_t width, uint32_t height)
	{
		this->DeclareAttachment(name, format, width, height, ImageOptions::DEFAULT);
	}

	void Pipeline::DeclareAttachment(const std::string& name, Format format, uint32_t width, uint32_t height, ImageOptions::Value options)
	{
		this->attachmentDeclarations.push_back(AttachmentDeclaration{
			name,
			format,
			width, 
			height,
			options
		});
	}

	void Pipeline::AddOutputAttachment(const std::string& name, ClearColor clear)
	{
		this->AddOutputAttachment(name, clear, OutputAttachment::ALL_LAYERS);
	}

	void Pipeline::AddOutputAttachment(const std::string& name, ClearDepthStencil clear)
	{
		this->AddOutputAttachment(name, clear, OutputAttachment::ALL_LAYERS);
	}

	void Pipeline::AddOutputAttachment(const std::string& name, AttachmentState onLoad)
	{
		this->AddOutputAttachment(name, onLoad, OutputAttachment::ALL_LAYERS);
	}

	void Pipeline::AddOutputAttachment(const std::string& name, ClearColor clear, uint32_t layer)
	{
		this->outputAttachments.push_back(OutputAttachment{
			name,
			clear,
			ClearDepthStencil{ },
			AttachmentState::CLEAR_COLOR,
			layer
		});
	}

	void Pipeline::AddOutputAttachment(const std::string& name, ClearDepthStencil clear, uint32_t layer)
	{
		this->outputAttachments.push_back(OutputAttachment{ 
			name, 
			ClearColor{ },
			clear, 
			AttachmentState::CLEAR_DEPTH_SPENCIL,
			layer
		});
	}

	void Pipeline::AddOutputAttachment(const std::string& name, AttachmentState onLoad, uint32_t layer)
	{
		this->outputAttachments.push_back(OutputAttachment{ 
			name, 
			ClearColor{ }, 
			ClearDepthStencil{ }, 
			onLoad,
			layer
		});
	}
}