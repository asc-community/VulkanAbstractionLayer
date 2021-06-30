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

#include "StringId.h"
#include "Buffer.h"
#include "Image.h"
#include "CommandBuffer.h"

namespace VulkanAbstractionLayer
{
    enum class AttachmentState
    {
        DISCARD_COLOR = 0,
        DISCARD_DEPTH_SPENCIL,
        LOAD_COLOR,
        LOAD_DEPTH_SPENCIL,
        CLEAR_COLOR,
        CLEAR_DEPTH_SPENCIL,
    };

	class DependencyStorage
	{
		struct BufferDependency
		{
			StringId Name;
            BufferUsage::Bits Usage;
		};

        struct ImageDependency
        {
            StringId Name;
            ImageUsage::Bits Usage;
        };

        struct AttachmentDependency
        {
            StringId Name;
            ClearColor ColorClear;
            ClearDepthSpencil DepthSpencilClear;
            AttachmentState OnLoad;
        };

        std::vector<BufferDependency> bufferDependencies;
        std::vector<ImageDependency> imageDependencies;
        std::vector<AttachmentDependency> attachmentDependencies;

    public:
        void AddBuffer(StringId name, BufferUsage::Bits usage);
        void AddImage(StringId name, ImageUsage::Bits usage);
        void AddAttachment(StringId name, ClearColor clear);
        void AddAttachment(StringId name, ClearDepthSpencil clear);
        void AddAttachment(StringId name, AttachmentState onLoad);

        const auto& GetBufferDependencies() const { return this->bufferDependencies; }
        const auto& GetImageDependencies() const { return this->imageDependencies; }
        const auto& GetAttachmentDependencies() const { return this->attachmentDependencies; }
	};
}