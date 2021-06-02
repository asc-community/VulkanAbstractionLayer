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

#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace VulkanAbstractionLayer
{
    ModelLoader::LoadedModel ModelLoader::LoadFromObj(const std::string& filepath)
    {
        LoadedModel result;

        tinyobj::ObjReaderConfig reader_config;
        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(filepath, reader_config))
        {
            return result;
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        result.Shapes.reserve(shapes.size());
        for (const auto& shape : shapes)
        {
            size_t index_offset = 0;
            auto& resultShape = result.Shapes.emplace_back();

            for (size_t faceIndex : shape.mesh.num_face_vertices)
            {
                for (size_t v = 0; v < faceIndex; v++)
                {
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                    auto& vertex = resultShape.Vertices.emplace_back();

                    vertex.Position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                    vertex.Position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                    vertex.Position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                    if (idx.texcoord_index >= 0)
                    {
                        vertex.TexCoord.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                        vertex.TexCoord.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    }

                    if (idx.normal_index >= 0) 
                    {
                        vertex.Normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                        vertex.Normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                        vertex.Normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    }
                }
                index_offset += faceIndex;
            }
        }

        return result;
    }
}