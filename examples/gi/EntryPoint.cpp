#include <filesystem>
#include <iostream>
#include <map>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
#include "VulkanAbstractionLayer/ModelLoader.h"
#include "VulkanAbstractionLayer/ImageLoader.h"
#include "VulkanAbstractionLayer/ImGuiRenderPass.h"
#include "VulkanAbstractionLayer/GraphicShader.h"

using namespace VulkanAbstractionLayer;

void VulkanInfoCallback(const std::string& message)
{
    std::cout << "[INFO Vulkan]: " << message << std::endl;
}

void VulkanErrorCallback(const std::string& message)
{
    std::cout << "[ERROR Vulkan]: " << message << std::endl;
}

void WindowErrorCallback(const std::string& message)
{
    std::cerr << "[ERROR Window]: " << message << std::endl;
}

constexpr size_t MaxMaterialCount = 256;
constexpr size_t MaxMeshCount = 256;
constexpr size_t ProbeResolution = 256;
constexpr Vector3 ProbeGridSize = { 1.0f, 1.0f, 3.0f };
Vector3 ProbeGridDensity = { 545.0f, 535.0f, 400.0 };
Vector3 ProbeGridOffset = { -50.0f, 600.0f, 50.0f };
bool DrawProbes = false;

struct Mesh
{
    struct Material
    {
        uint32_t AlbedoIndex;
        uint32_t NormalIndex;
        uint32_t MetallicRoughnessIndex;
        float RoughnessScale;
        float MetallicScale;
        uint32_t Padding[3];
    };

    struct Submesh
    {
        Buffer VertexBuffer;
        Buffer IndexBuffer;
        uint32_t MaterialIndex;
    };

    std::vector<Submesh> Submeshes;
    std::vector<Material> Materials;
    std::vector<Image> Textures;

    struct MeshData
    {
        Matrix4x4 Transform = Matrix4x4(1.0f);
    } Data;
};

struct CameraUniformData
{
    Matrix4x4 Matrix;
    Vector3 Position;
};

struct ReflectionProbeUniformData
{
    std::array<Matrix4x4, 6> Matrices;
};

struct Camera
{
    Vector3 Position{ 40.0f, 200.0f, -90.0f };
    Vector2 Rotation{ Pi, 0.0f };
    float Fov = 65.0f;
    float MovementSpeed = 250.0f;
    float RotationMovementSpeed = 2.5f;
    float AspectRatio = 16.0f / 9.0f;
    float ZNear = 0.5f;
    float ZFar = 100000.0f;

    void Rotate(const Vector2& delta)
    {
        this->Rotation += this->RotationMovementSpeed * delta;

        constexpr float MaxAngleY = HalfPi - 0.001f;
        constexpr float MaxAngleX = TwoPi;
        this->Rotation.y = std::clamp(this->Rotation.y, -MaxAngleY, MaxAngleY);
        this->Rotation.x = std::fmod(this->Rotation.x, MaxAngleX);
    }

    void Move(const Vector3& direction)
    {
        Matrix3x3 view{
            std::sin(Rotation.x), 0.0f, std::cos(Rotation.x), // forward
            0.0f, 1.0f, 0.0f, // up
            std::sin(Rotation.x - HalfPi), 0.0f, std::cos(Rotation.x - HalfPi) // right
        };

        this->Position += this->MovementSpeed * (view * direction);
    }

    Matrix4x4 GetViewMatrix() const
    {
        Vector3 direction{
            std::cos(this->Rotation.y) * std::sin(this->Rotation.x),
            std::sin(this->Rotation.y),
            std::cos(this->Rotation.y) * std::cos(this->Rotation.x)
        };
        return MakeLookAtMatrix(this->Position, direction, Vector3{0.0f, 1.0f, 0.0f});
    }

    Matrix4x4 GetProjectionMatrix() const
    {
        return MakePerspectiveMatrix(ToRadians(this->Fov), this->AspectRatio, this->ZNear, this->ZFar);
    }

    Matrix4x4 GetMatrix()
    {
        return this->GetProjectionMatrix() * this->GetViewMatrix();
    }
};

struct ReflectionProbesData
{
    std::vector<Image> Cubemaps;
    std::vector<Vector4> Positions;
};

struct SharedResources
{
    Buffer CameraUniformBuffer;
    Buffer MeshDataUniformBuffer;
    Buffer MaterialUniformBuffer;
    Buffer ReflectionProbeUniformBuffer;
    std::vector<Mesh> WorldMeshes;
    Mesh Sphere;
    CameraUniformData CameraUniform;
    ReflectionProbeUniformData ReflectionProbeUniform;
    ReflectionProbesData ReflectionProbes;
    size_t CurrentProbeIndex;
    Image Skybox;
    Image SkyboxIrradiance;
    Image BRDFLUT;
    std::shared_ptr<Shader> MainShader;
};

void LoadImage(CommandBuffer& commandBuffer, Image& image, const ImageData& imageData, ImageOptions::Value options)
{
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    image.Init(
        imageData.Width,
        imageData.Height,
        imageData.ImageFormat,
        ImageUsage::SHADER_READ | ImageUsage::TRANSFER_SOURCE | ImageUsage::TRANSFER_DISTINATION,
        MemoryUsage::GPU_ONLY,
        options
    );
    auto allocation = stageBuffer.Submit(MakeView(imageData.ByteData));
    commandBuffer.CopyBufferToImage(
        BufferInfo{ stageBuffer.GetBuffer(), allocation.Offset }, 
        ImageInfo{ image, ImageUsage::UNKNOWN, 0, 0 }
    );
    if (options & ImageOptions::MIPMAPS)
    {
        if (imageData.MipLevels.empty())
        {
            commandBuffer.GenerateMipLevels(image, ImageUsage::TRANSFER_DISTINATION, BlitFilter::LINEAR);
        }
        else
        {
            uint32_t mipLevel = 1;
            for (const auto& mipData : imageData.MipLevels)
            {
                auto allocation = stageBuffer.Submit(MakeView(mipData));
                commandBuffer.CopyBufferToImage(
                    BufferInfo{ stageBuffer.GetBuffer(), allocation.Offset },
                    ImageInfo{ image, ImageUsage::TRANSFER_DISTINATION, mipLevel, 0 }
                );
                mipLevel++;
            }
        }
    }
    commandBuffer.TransferLayout(image, ImageUsage::TRANSFER_DISTINATION, ImageUsage::SHADER_READ);
}

void LoadImage(Image& image, const std::string& filepath, ImageOptions::Value options)
{
    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
    commandBuffer.Begin();

    LoadImage(commandBuffer, image, ImageLoader::LoadImageFromFile(filepath), options);

    stageBuffer.Flush();
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();
}

void LoadCubemap(Image& image, const std::string& filepath)
{
    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    commandBuffer.Begin();

    auto cubemapData = ImageLoader::LoadCubemapImageFromFile(filepath);
    image.Init(
        cubemapData.FaceWidth,
        cubemapData.FaceHeight,
        cubemapData.FaceFormat,
        ImageUsage::TRANSFER_DISTINATION | ImageUsage::TRANSFER_SOURCE | ImageUsage::SHADER_READ,
        MemoryUsage::GPU_ONLY,
        ImageOptions::CUBEMAP | ImageOptions::MIPMAPS
    );

    for (uint32_t layer = 0; layer < cubemapData.Faces.size(); layer++)
    {
        const auto& face = cubemapData.Faces[layer];
        auto textureAllocation = stageBuffer.Submit(face.data(), face.size());

        commandBuffer.CopyBufferToImage(
            BufferInfo{ stageBuffer.GetBuffer(), textureAllocation.Offset },
            ImageInfo{ image, ImageUsage::UNKNOWN, 0, layer }
        );
    }

    commandBuffer.GenerateMipLevels(image, ImageUsage::TRANSFER_DISTINATION, BlitFilter::LINEAR);
    commandBuffer.TransferLayout(image, ImageUsage::TRANSFER_DISTINATION, ImageUsage::SHADER_READ);

    stageBuffer.Flush();
    commandBuffer.End();

    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();
}

void LoadModel(Mesh& mesh, const std::string& filepath)
{
    auto model = ModelLoader::Load(filepath);
   
    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    commandBuffer.Begin();

    for (const auto& shape : model.Shapes)
    {
        auto& submesh = mesh.Submeshes.emplace_back();
        submesh.VertexBuffer.Init(
            shape.Vertices.size() * sizeof(ModelData::Vertex),
            BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, 
            MemoryUsage::GPU_ONLY
        );

        auto vertexAllocation = stageBuffer.Submit(MakeView(shape.Vertices));
        commandBuffer.CopyBuffer(
            BufferInfo{ stageBuffer.GetBuffer(), vertexAllocation.Offset },
            BufferInfo{ submesh.VertexBuffer, 0 }, 
            vertexAllocation.Size
        );

        submesh.IndexBuffer.Init(
            shape.Indices.size() * sizeof(ModelData::Index),
            BufferUsage::INDEX_BUFFER | BufferUsage::TRANSFER_DESTINATION,
            MemoryUsage::GPU_ONLY
        );

        auto indexAllocation = stageBuffer.Submit(MakeView(shape.Indices));
        commandBuffer.CopyBuffer(
            BufferInfo{ stageBuffer.GetBuffer(), indexAllocation.Offset },
            BufferInfo{ submesh.IndexBuffer, 0 },
            indexAllocation.Size
        );

        submesh.MaterialIndex = shape.MaterialIndex;
    }

    stageBuffer.Flush();
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();

    uint32_t textureIndex = 0;
    for (const auto& material : model.Materials)
    {
        commandBuffer.Begin();

        LoadImage(commandBuffer, mesh.Textures.emplace_back(), material.AlbedoTexture, ImageOptions::MIPMAPS);
        LoadImage(commandBuffer, mesh.Textures.emplace_back(), material.NormalTexture, ImageOptions::MIPMAPS);
        LoadImage(commandBuffer, mesh.Textures.emplace_back(), material.MetallicRoughness, ImageOptions::MIPMAPS);

        mesh.Materials.push_back(Mesh::Material{ textureIndex, textureIndex + 1, textureIndex + 2, 1.0f, 1.0f });
        textureIndex += 3;

        stageBuffer.Flush();
        commandBuffer.End();
        GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
        stageBuffer.Reset();
    }
}

class UniformSubmitRenderPass : public RenderPass
{
    SharedResources& sharedResources;

    std::vector<Mesh::Material> materials;
    std::vector<Mesh::MeshData> meshDatas;
public:
    UniformSubmitRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.AddDependency("CameraUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
        pipeline.AddDependency("MeshDataUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
        pipeline.AddDependency("MaterialUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
        pipeline.AddDependency("ReflectionProbeUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
    }

    virtual void ResolveResources(ResolveState resolve) override
    {
        resolve.Resolve("CameraUniformBuffer", this->sharedResources.CameraUniformBuffer);
        resolve.Resolve("MeshDataUniformBuffer", this->sharedResources.MeshDataUniformBuffer);
        resolve.Resolve("MaterialUniformBuffer", this->sharedResources.MaterialUniformBuffer);
        resolve.Resolve("ReflectionProbeUniformBuffer", this->sharedResources.ReflectionProbeUniformBuffer);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto FillUniform = [&state](const auto& uniformData, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(&uniformData);
            state.Commands.CopyBuffer(
                BufferInfo{ stageBuffer.GetBuffer(), uniformAllocation.Offset }, 
                BufferInfo{ uniformBuffer, 0 },
                uniformAllocation.Size
            );
        };
        auto FillUniformArray = [&state](const auto& uniformArray, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(MakeView(uniformArray));
            state.Commands.CopyBuffer(
                BufferInfo{ stageBuffer.GetBuffer(), uniformAllocation.Offset }, 
                BufferInfo{ uniformBuffer, 0 }, 
                uniformAllocation.Size
            );
        };

        this->meshDatas.clear();
        for (const auto& mesh : this->sharedResources.WorldMeshes)
            this->meshDatas.push_back(mesh.Data);

        this->materials.clear();
        for (const auto& mesh : this->sharedResources.WorldMeshes)
            for (const auto& material : mesh.Materials)
                this->materials.push_back(material);

        FillUniform(this->sharedResources.CameraUniform, this->sharedResources.CameraUniformBuffer);
        FillUniformArray(this->meshDatas, this->sharedResources.MeshDataUniformBuffer);
        FillUniformArray(this->materials, this->sharedResources.MaterialUniformBuffer);
        FillUniform(this->sharedResources.ReflectionProbeUniform, this->sharedResources.ReflectionProbeUniformBuffer);
    }
};

class ReflectionProbeCopyRenderPass : public RenderPass
{
    SharedResources& sharedResources;
public:
    ReflectionProbeCopyRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources) { }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.AddDependency("OutputProbe", ImageUsage::TRANSFER_SOURCE);
        pipeline.AddDependency("ReflectionProbesCubemaps", ImageUsage::TRANSFER_DISTINATION);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& renderedProbe = state.GetAttachment("OutputProbe");
        auto& reflectionProbe = this->sharedResources.ReflectionProbes.Cubemaps[this->sharedResources.CurrentProbeIndex];
        for (uint32_t layer = 0; layer < renderedProbe.GetLayerCount(); layer++)
        {
            state.Commands.CopyImage(
                ImageInfo{ renderedProbe, ImageUsage::TRANSFER_SOURCE, 0, layer },
                ImageInfo{ reflectionProbe, ImageUsage::TRANSFER_DISTINATION, 0, layer }
            );
        }
        state.Commands.GenerateMipLevels(reflectionProbe, ImageUsage::TRANSFER_DISTINATION, BlitFilter::LINEAR);
    }
};

class ReflectionProbeCalculateRenderPass : public RenderPass
{
    SharedResources& sharedResources;
    std::vector<ImageReference> textureArray;
    std::vector<uint32_t> materialIndexOffsets;
    std::vector<uint32_t> textureIndexOffsets;
    Sampler TextureSampler;
public:

    ReflectionProbeCalculateRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->TextureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

        uint32_t totalMaterials = 0;
        uint32_t totalTextures = 0;
        for (const auto& mesh : this->sharedResources.WorldMeshes)
        {
            this->materialIndexOffsets.push_back(totalMaterials);
            this->textureIndexOffsets.push_back(totalTextures);
            for (const auto& texture : mesh.Textures)
                this->textureArray.push_back(std::ref(texture));
            totalMaterials += mesh.Materials.size();
            totalTextures += mesh.Textures.size();
        }
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("probe_main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                VertexBinding::BindingRangeAll
            }
        };

        pipeline.DeclareAttachment("OutputProbe", Format::R8G8B8A8_UNORM, ProbeResolution, ProbeResolution, ImageOptions::CUBEMAP);
        pipeline.DeclareAttachment("OutputProbeDepth", Format::D32_SFLOAT_S8_UINT, ProbeResolution, ProbeResolution, ImageOptions::CUBEMAP);

        pipeline.DescriptorBindings
            .Bind(0, "CameraUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(1, "ReflectionProbeUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(2, "MeshDataUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(3, "MaterialUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(4, "TextureArray", UniformType::SAMPLED_IMAGE)
            .Bind(5, this->TextureSampler, UniformType::SAMPLER)
            .Bind(6, "BRDFLUT", this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(7, "ReflectionProbesCubemaps", UniformType::SAMPLED_IMAGE)
            .Bind(8, "Skybox", UniformType::SAMPLED_IMAGE)
            .Bind(9, "SkyboxIrradiance", UniformType::SAMPLED_IMAGE);

        pipeline.AddOutputAttachment("OutputProbe", ClearColor{ 0.05f, 0.0f, 0.1f, 1.0f });
        pipeline.AddOutputAttachment("OutputProbeDepth", ClearDepthStencil{ });
    }

    virtual void ResolveResources(ResolveState resolve) override
    {
        resolve.Resolve("TextureArray", this->textureArray);
        resolve.Resolve("BRDFLUT", this->sharedResources.BRDFLUT);
        resolve.Resolve("Skybox", this->sharedResources.Skybox);
        resolve.Resolve("SkyboxIrradiance", this->sharedResources.SkyboxIrradiance);
        resolve.Resolve("ReflectionProbesCubemaps", this->sharedResources.ReflectionProbes.Cubemaps);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("OutputProbe");
        state.Commands.SetRenderArea(output);

        struct
        {
            Vector3 CameraPosition;
            uint32_t MaterialIndex;
            Vector3 ProbeGridOffset;
            uint32_t ModelIndex;
            Vector3 ProbeGridDensity;
            uint32_t TextureOffset;
            Vector3 ProbeGridSize;
        } pushConstants;

        size_t meshIndex = 0;
        for (const auto& mesh : this->sharedResources.WorldMeshes)
        {
            for (const auto& submesh : mesh.Submeshes)
            {
                pushConstants.CameraPosition = this->sharedResources.ReflectionProbes.Positions[this->sharedResources.CurrentProbeIndex];
                pushConstants.MaterialIndex = this->materialIndexOffsets[meshIndex] + submesh.MaterialIndex;
                pushConstants.TextureOffset = this->textureIndexOffsets[meshIndex];
                pushConstants.ProbeGridSize = ProbeGridSize;
                pushConstants.ModelIndex = meshIndex;
                pushConstants.ProbeGridDensity = ProbeGridDensity;
                pushConstants.ProbeGridOffset = ProbeGridOffset;

                size_t indexCount = submesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);
                state.Commands.PushConstants(state.Pass, &pushConstants);
                state.Commands.BindVertexBuffers(submesh.VertexBuffer);
                state.Commands.BindIndexBufferUInt32(submesh.IndexBuffer);
                state.Commands.DrawIndexed((uint32_t)indexCount, 1);
            }
            meshIndex++;
        }
    }
};

class OpaqueRenderPass : public RenderPass
{    
    SharedResources& sharedResources;
    std::vector<ImageReference> textureArray;
    std::vector<ImageReference> reflectionProbeArray;
    std::vector<uint32_t> materialIndexOffsets;
    std::vector<uint32_t> textureIndexOffsets;
public:
    Sampler TextureSampler;

    OpaqueRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->TextureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

        uint32_t totalMaterials = 0;
        uint32_t totalTextures = 0;
        for (const auto& mesh : this->sharedResources.WorldMeshes)
        {
            this->materialIndexOffsets.push_back(totalMaterials);
            this->textureIndexOffsets.push_back(totalTextures);
            for (const auto& texture : mesh.Textures)
                this->textureArray.push_back(std::ref(texture));
            totalMaterials += mesh.Materials.size();
            totalTextures += mesh.Textures.size();
        }
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = this->sharedResources.MainShader;

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                VertexBinding::BindingRangeAll
            }
        };

        pipeline.DeclareAttachment("Output", Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth", Format::D32_SFLOAT_S8_UINT);

        pipeline.DescriptorBindings
            .Bind(0, "CameraUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(1, "ReflectionProbeUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(2, "MeshDataUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(3, "MaterialUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(4, "TextureArray", UniformType::SAMPLED_IMAGE)
            .Bind(5, this->TextureSampler, UniformType::SAMPLER)
            .Bind(6, "BRDFLUT", this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(7, "ReflectionProbesCubemaps", UniformType::SAMPLED_IMAGE)
            .Bind(8, "Skybox", UniformType::SAMPLED_IMAGE)
            .Bind(9, "SkyboxIrradiance", UniformType::SAMPLED_IMAGE);

        pipeline.AddOutputAttachment("Output", ClearColor{ 0.05f, 0.0f, 0.1f, 1.0f });
        pipeline.AddOutputAttachment("OutputDepth", ClearDepthStencil{ });
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output");
        state.Commands.SetRenderArea(output);

        struct
        {
            Vector3 CameraPosition;
            uint32_t MaterialIndex;
            Vector3 ProbeGridOffset;
            uint32_t ModelIndex;
            Vector3 ProbeGridDensity;
            uint32_t TextureOffset;
            Vector3 ProbeGridSize;
        } pushConstants;

        size_t meshIndex = 0;
        for (const auto& mesh : this->sharedResources.WorldMeshes)
        {
            for (const auto& submesh : mesh.Submeshes)
            {
                pushConstants.CameraPosition = this->sharedResources.CameraUniform.Position;
                pushConstants.MaterialIndex = this->materialIndexOffsets[meshIndex] + submesh.MaterialIndex;
                pushConstants.TextureOffset = this->textureIndexOffsets[meshIndex];
                pushConstants.ProbeGridSize = ProbeGridSize;
                pushConstants.ModelIndex = meshIndex;
                pushConstants.ProbeGridDensity = ProbeGridDensity;
                pushConstants.ProbeGridOffset = ProbeGridOffset;

                size_t indexCount = submesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);
                state.Commands.PushConstants(state.Pass, &pushConstants);
                state.Commands.BindVertexBuffers(submesh.VertexBuffer);
                state.Commands.BindIndexBufferUInt32(submesh.IndexBuffer);
                state.Commands.DrawIndexed((uint32_t)indexCount, 1);
            }
            meshIndex++;
        }
    }
};

class ReflectionProbeDebugRenderPass : public RenderPass
{
    SharedResources& sharedResources;
    Sampler textureSampler;
public:
    ReflectionProbeDebugRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->textureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("reflection_probe_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("reflection_probe_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                VertexBinding::BindingRangeAll
            }
        };

        pipeline.DescriptorBindings
            .Bind(0, "CameraUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(1, "ReflectionProbesCubemaps", UniformType::SAMPLED_IMAGE)
            .Bind(2, this->textureSampler, UniformType::SAMPLER);

        pipeline.AddOutputAttachment("Output", AttachmentState::LOAD_COLOR);
        pipeline.AddOutputAttachment("OutputDepth", AttachmentState::LOAD_DEPTH_SPENCIL);
    }

    virtual void OnRender(RenderPassState state) override
    {
        if (!DrawProbes) return;

        auto& output = state.GetAttachment("Output");
        state.Commands.SetRenderArea(output);

        struct
        {
            Vector3 Position;
            float Size;
            uint32_t ProbeIndex;
        } pushConstants;

        uint32_t probeIndex = 0;
        for (const auto& probePosition : this->sharedResources.ReflectionProbes.Positions)
        {
            const auto& sphereMesh = this->sharedResources.Sphere.Submeshes[0];
            size_t indexCount = sphereMesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);

            pushConstants.Position = probePosition;
            pushConstants.Size = 10.0f;
            pushConstants.ProbeIndex = probeIndex++;

            state.Commands.PushConstants(state.Pass, &pushConstants);
            state.Commands.BindVertexBuffers(sphereMesh.VertexBuffer);
            state.Commands.BindIndexBufferUInt32(sphereMesh.IndexBuffer);
            state.Commands.DrawIndexed((uint32_t)indexCount, 1);
        }
    }
};

class SkyboxRenderPass : public RenderPass
{
    SharedResources& sharedResources;
    Sampler skyboxSampler;
public:
    SkyboxRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->skyboxSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::CLAMP_TO_EDGE, Sampler::MipFilter::LINEAR);
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("skybox_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("skybox_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.DescriptorBindings
            .Bind(0, "CameraUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(1, "Skybox", this->skyboxSampler, UniformType::COMBINED_IMAGE_SAMPLER);

        pipeline.AddOutputAttachment("Output", AttachmentState::LOAD_COLOR);
        pipeline.AddOutputAttachment("OutputDepth", AttachmentState::LOAD_DEPTH_SPENCIL);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output");
        state.Commands.SetRenderArea(output);

        constexpr uint32_t SkyboxVertexCount = 36;
        state.Commands.Draw(SkyboxVertexCount, 1);
    }
};

class ReflectionProbeSkyboxRenderPass : public RenderPass
{
    SharedResources& sharedResources;
    Sampler skyboxSampler;
public:
    ReflectionProbeSkyboxRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->skyboxSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::CLAMP_TO_EDGE, Sampler::MipFilter::LINEAR);
    }
    
    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("probe_skybox_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("skybox_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.DescriptorBindings
            .Bind(1, "Skybox", this->skyboxSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(2, "ReflectionProbeUniformBuffer", UniformType::UNIFORM_BUFFER);

        pipeline.AddOutputAttachment("OutputProbe", AttachmentState::LOAD_COLOR);
        pipeline.AddOutputAttachment("OutputProbeDepth", AttachmentState::LOAD_DEPTH_SPENCIL);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("OutputProbe");
        state.Commands.SetRenderArea(output);

        constexpr uint32_t SkyboxVertexCount = 36;
        state.Commands.PushConstants(state.Pass, &this->sharedResources.ReflectionProbes.Positions[this->sharedResources.CurrentProbeIndex]);
        state.Commands.Draw(SkyboxVertexCount, 1);
    }
};

auto CreateRenderGraph(SharedResources& resources)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass", std::make_unique<UniformSubmitRenderPass>(resources))
        .AddRenderPass("ReflectionProbeCalculatePass", std::make_unique<ReflectionProbeCalculateRenderPass>(resources))
        .AddRenderPass("ReflectionProbeSkyboxPass", std::make_unique<ReflectionProbeSkyboxRenderPass>(resources))
        .AddRenderPass("OpaquePass", std::make_unique<OpaqueRenderPass>(resources))
        .AddRenderPass("ReflectionProbeCopyPass", std::make_unique<ReflectionProbeCopyRenderPass>(resources))
        .AddRenderPass("ReflectionProbeDebugPass", std::make_unique<ReflectionProbeDebugRenderPass>(resources))
        .AddRenderPass("SkyboxPass", std::make_unique<SkyboxRenderPass>(resources))
        .AddRenderPass("ImGuiPass", std::make_unique<ImGuiRenderPass>("Output"))
        .SetOutputName("Output");

    return renderGraphBuilder.Build();
}

Matrix4x4 GetReflectionProbeMatrix(const Vector3& position, uint32_t layer)
{
    Camera camera;
    camera.Position = position;
    camera.AspectRatio = 1.0f;
    camera.Fov = 90.0f;
    if (layer == 0)
        camera.Rotation = { HalfPi, 0.0f };
    if (layer == 1)
        camera.Rotation = { -HalfPi, 0.0f };
    if (layer == 2)
        camera.Rotation = { Pi, HalfPi - 0.001f };
    if (layer == 3)
        camera.Rotation = { Pi, -HalfPi + 0.001f };
    if (layer == 4)
        camera.Rotation = { Pi, 0.0f };
    if (layer == 5)
        camera.Rotation = { 0.0f, 0.0f };

    return camera.GetMatrix();
}

void AddReflectionProbe(CommandBuffer& commandBuffer, ReflectionProbesData& probes, const Vector3& position, const Image& defaultCubemap)
{
    probes.Positions.push_back(Vector4(position, 0.0f));
    auto& probe = probes.Cubemaps.emplace_back(
        ProbeResolution,
        ProbeResolution, 
        Format::R8G8B8A8_UNORM, 
        ImageUsage::TRANSFER_DISTINATION | ImageUsage::TRANSFER_SOURCE | ImageUsage::SHADER_READ, 
        MemoryUsage::GPU_ONLY, 
        ImageOptions::CUBEMAP | ImageOptions::MIPMAPS
    );
    commandBuffer.BlitImage(defaultCubemap, ImageUsage::TRANSFER_SOURCE, probe, ImageUsage::UNKNOWN, BlitFilter::LINEAR);
}

int main()
{
    if (std::filesystem::exists(APPLICATION_WORKING_DIRECTORY))
        std::filesystem::current_path(APPLICATION_WORKING_DIRECTORY);

    WindowCreateOptions windowOptions;
    windowOptions.Position = { 100.0f, 100.0f };
    windowOptions.Size = { 1728.0f, 972.0f };
    windowOptions.ErrorCallback = WindowErrorCallback;

    Window window(windowOptions);

    VulkanContextCreateOptions vulkanOptions;
    vulkanOptions.VulkanApiMajorVersion = 1;
    vulkanOptions.VulkanApiMinorVersion = 2;
    vulkanOptions.Extensions = window.GetRequiredExtensions();
    vulkanOptions.Layers = { "VK_LAYER_KHRONOS_validation" };
    vulkanOptions.ErrorCallback = VulkanErrorCallback;
    vulkanOptions.InfoCallback = VulkanInfoCallback;

    VulkanContext Vulkan(vulkanOptions);
    SetCurrentVulkanContext(Vulkan);

    ContextInitializeOptions deviceOptions;
    deviceOptions.PreferredDeviceType = DeviceType::DISCRETE_GPU;
    deviceOptions.ErrorCallback = VulkanErrorCallback;
    deviceOptions.InfoCallback = VulkanInfoCallback;

    Vulkan.InitializeContext(window.CreateWindowSurface(Vulkan), deviceOptions);

    SharedResources sharedResources{
        Buffer{ sizeof(CameraUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(Mesh::MeshData) * MaxMeshCount, BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(Mesh::Material) * MaxMaterialCount, BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(ReflectionProbeUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { }, // sponza
        { }, // sphere
        { }, // camera uniform
        { }, // reflection probe uniform
        { }, // reflection probes
        { }, // current probe index
        { }, // skybox
        { }, // skybox irradiance
        { }, // brdf lut
        std::make_shared<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        ),
    };

    LoadCubemap(sharedResources.Skybox, "../textures/skybox.png");
    LoadCubemap(sharedResources.SkyboxIrradiance, "../textures/skybox_irradiance.png");
    LoadImage(sharedResources.BRDFLUT, "../textures/brdf_lut.dds", ImageOptions::DEFAULT);

    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    commandBuffer.Begin();
    commandBuffer.TransferLayout(sharedResources.Skybox, ImageUsage::SHADER_READ, ImageUsage::TRANSFER_SOURCE);
    for(int x = -ProbeGridSize.x; x <= ProbeGridSize.x; x++)
        for(int y = -ProbeGridSize.y; y <= ProbeGridSize.y; y++)
            for (int z = -ProbeGridSize.z; z <= ProbeGridSize.z; z++)
            {
                Vector3 offset = ProbeGridOffset + Vector3{ (float)x, (float)y, (float)z } * ProbeGridDensity;
                AddReflectionProbe(commandBuffer, sharedResources.ReflectionProbes, offset, sharedResources.Skybox);
            }
    commandBuffer.TransferLayout(sharedResources.Skybox, ImageUsage::TRANSFER_SOURCE, ImageUsage::SHADER_READ);
    commandBuffer.TransferLayout(sharedResources.ReflectionProbes.Cubemaps, ImageUsage::TRANSFER_DISTINATION, ImageUsage::SHADER_READ);
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);

    LoadModel(sharedResources.Sphere, "../models/sphere/sphere.obj");

    auto& cubeMesh = sharedResources.WorldMeshes.emplace_back();
    LoadModel(cubeMesh, "../models/cube/cube.obj");
    cubeMesh.Data.Transform = MakeScaleMatrix(Vector3{ 100.0f, 100.0f, 100.0f });
    cubeMesh.Data.Transform[3] = Vector4{ 0.0f, 50.0f, 0.0f, 1.0f };
    cubeMesh.Submeshes[0].MaterialIndex = 0;
    cubeMesh.Materials.push_back(Mesh::Material{ 0, 1, 2, 0.0f, 1.0f });
    LoadImage(cubeMesh.Textures.emplace_back(), "../textures/default_albedo.png", ImageOptions::MIPMAPS);
    LoadImage(cubeMesh.Textures.emplace_back(), "../textures/default_normal.png", ImageOptions::MIPMAPS);
    LoadImage(cubeMesh.Textures.emplace_back(), "../textures/default_metallic_roughness.png", ImageOptions::MIPMAPS);

    auto& sponzaMesh = sharedResources.WorldMeshes.emplace_back();
    sponzaMesh.Data.Transform = MakeRotationMatrix(Vector3{ 0.0f, HalfPi - 0.01f, 0.0f });
    LoadModel(sponzaMesh, "../models/Sponza/glTF/Sponza.gltf");

    Sampler ImGuiImageSampler(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

    std::unique_ptr<RenderGraph> renderGraph = CreateRenderGraph(sharedResources);

    Camera camera;

    window.OnResize([&Vulkan, &sharedResources, &renderGraph, &camera](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(sharedResources);
        camera.AspectRatio = size.x / size.y;
    });
    
    ImGuiVulkanContext::Init(window, renderGraph->GetNodeByName("ImGuiPass").PassNative.RenderPassHandle);

    std::map<VkImage, ImTextureID> ImGuiRegisteredImages;
    for (const auto& mesh : sharedResources.WorldMeshes)
    {
        for (const auto& material : mesh.Materials)
        {
            auto& albedoTexture = mesh.Textures[material.AlbedoIndex];
            auto& normalTexture = mesh.Textures[material.NormalIndex];
            auto& metallicRoughnessTexture = mesh.Textures[material.MetallicRoughnessIndex];
            
            if (ImGuiRegisteredImages.find(albedoTexture.GetNativeHandle()) == ImGuiRegisteredImages.end())
                ImGuiRegisteredImages.emplace(
                    albedoTexture.GetNativeHandle(),
                    ImGuiVulkanContext::RegisterImage(albedoTexture, ImGuiImageSampler)
                );
            if (ImGuiRegisteredImages.find(normalTexture.GetNativeHandle()) == ImGuiRegisteredImages.end())
                ImGuiRegisteredImages.emplace(
                    normalTexture.GetNativeHandle(),
                    ImGuiVulkanContext::RegisterImage(normalTexture, ImGuiImageSampler)
                );
            if (ImGuiRegisteredImages.find(metallicRoughnessTexture.GetNativeHandle()) == ImGuiRegisteredImages.end())
                ImGuiRegisteredImages.emplace(
                    metallicRoughnessTexture.GetNativeHandle(),
                    ImGuiVulkanContext::RegisterImage(metallicRoughnessTexture, ImGuiImageSampler)
                );
        }
    }

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (Vulkan.IsRenderingEnabled())
        {
            Vulkan.StartFrame();
            ImGuiVulkanContext::StartFrame();

            auto dt = ImGui::GetIO().DeltaTime;

            sharedResources.CurrentProbeIndex = (sharedResources.CurrentProbeIndex + 1) % sharedResources.ReflectionProbes.Cubemaps.size();

            auto mouseMovement = ImGui::GetMouseDragDelta(MouseButton::RIGHT, 0.0f);
            ImGui::ResetMouseDragDelta(MouseButton::RIGHT);
            camera.Rotate(Vector2{ -mouseMovement.x, -mouseMovement.y } *dt);

            Vector3 movementDirection{ 0.0f };
            if (ImGui::IsKeyDown(KeyCode::W))
                movementDirection += Vector3{ 1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::A))
                movementDirection += Vector3{ 0.0f,  0.0f, -1.0f };
            if (ImGui::IsKeyDown(KeyCode::S))
                movementDirection += Vector3{ -1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::D))
                movementDirection += Vector3{ 0.0f,  0.0f,  1.0f };
            if (ImGui::IsKeyDown(KeyCode::SPACE))
                movementDirection += Vector3{ 0.0f,  1.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::LEFT_SHIFT))
                movementDirection += Vector3{ 0.0f, -1.0f,  0.0f };
            if (movementDirection != Vector3{ 0.0f }) movementDirection = Normalize(movementDirection);
            camera.Move(movementDirection * dt);

            ImGui::Begin("Camera");
            ImGui::DragFloat("movement speed", &camera.MovementSpeed, 0.1f);
            ImGui::DragFloat("rotation movement speed", &camera.RotationMovementSpeed, 0.1f);
            ImGui::DragFloat3("position", &camera.Position[0]);
            ImGui::DragFloat2("rotation", &camera.Rotation[0], 0.01f);
            ImGui::DragFloat("fov", &camera.Fov);
            ImGui::End();

            sharedResources.CameraUniform.Matrix = camera.GetMatrix();
            sharedResources.CameraUniform.Position = camera.Position;

            ImGui::Begin("Performace");
            ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
            ImGui::End();

            ImGui::Begin("meshes");
            int meshIndex = 0;
            for (auto& mesh : sharedResources.WorldMeshes)
            {
                ImGui::PushID(meshIndex++);

                auto& transform = mesh.Data.Transform;
                Vector3 position = transform[3];
                Vector3 scale = Vector3{ Length(transform[0]), Length(transform[1]), Length(transform[2]) };
                Matrix3x4 rotationMatrix = transform * MakeScaleMatrix(1.0f / scale);
                Vector3 rotation = MakeRotationAngles(rotationMatrix);

                ImGui::DragFloat3("position", &position[0]);
                ImGui::DragFloat3("rotation", &rotation[0], 0.01f);
                ImGui::DragFloat3("scale",    &scale[0], 1.0f, 0.0f, 100000.0f);

                // update transform reference
                transform = MakeRotationMatrix(rotation);
                transform[0] *= scale.x;
                transform[1] *= scale.y;
                transform[2] *= scale.z;
                transform[3] = Vector4(position, 1.0f);

                ImGui::PopID();
                ImGui::Separator();
            }
            ImGui::End();

            int materialIndex = 0;
            ImGui::Begin("materials");
            for (auto& mesh : sharedResources.WorldMeshes)
            {
                for (auto& material : mesh.Materials)
                {
                    ImGui::PushID(materialIndex++);

                    ImGui::BeginTable(("material_" + std::to_string(materialIndex)).c_str(), 4);

                    ImGui::TableSetupColumn("roughness");
                    ImGui::TableSetupColumn("albedo image");
                    ImGui::TableSetupColumn("normal image");
                    ImGui::TableSetupColumn("metallic-roughness image");
                    ImGui::TableHeadersRow();

                    ImGui::TableNextColumn();
                    ImGui::DragFloat("roughness scale", &material.RoughnessScale, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("metallic scale", &material.MetallicScale, 0.01f, 0.0f, 1.0f);
                    ImGui::TableNextColumn();
                    ImGui::Image(ImGuiRegisteredImages.at(mesh.Textures[material.AlbedoIndex].GetNativeHandle()), { 128.0f, 128.0f });
                    ImGui::TableNextColumn();
                    ImGui::Image(ImGuiRegisteredImages.at(mesh.Textures[material.NormalIndex].GetNativeHandle()), { 128.0f, 128.0f });
                    ImGui::TableNextColumn();
                    ImGui::Image(ImGuiRegisteredImages.at(mesh.Textures[material.MetallicRoughnessIndex].GetNativeHandle()), { 128.0f, 128.0f });

                    ImGui::EndTable();

                    ImGui::Separator();
                    ImGui::PopID();
                }
            }
            ImGui::End();

            ImGui::Begin("Reflection probes");
            int reflectionProbeIndex = 0;

            bool gridInvalidated = false;
            ImGui::Checkbox("draw probes", &DrawProbes);
            gridInvalidated |= ImGui::DragFloat3("grid density", &ProbeGridDensity[0]);
            gridInvalidated |= ImGui::DragFloat3("grid offset", &ProbeGridOffset[0]);
            if (gridInvalidated)
            {
                size_t probeIndex = 0;
                for (int x = -ProbeGridSize.x; x <= ProbeGridSize.x; x++)
                    for (int y = -ProbeGridSize.y; y <= ProbeGridSize.y; y++)
                        for (int z = -ProbeGridSize.z; z <= ProbeGridSize.z; z++)
                        {
                            Vector3 offset = ProbeGridOffset + Vector3{ (float)x, (float)y, (float)z } * ProbeGridDensity;
                            sharedResources.ReflectionProbes.Positions[probeIndex] = Vector4(offset, 0.0f);
                            probeIndex++;
                        }
            }

            if (ImGui::TreeNode("probes"))
            {
                for (auto& probePosition : sharedResources.ReflectionProbes.Positions)
                {
                    ImGui::PushID(reflectionProbeIndex++);
                    ImGui::DragFloat3("position", &probePosition[0]);
                    ImGui::PopID();
                    ImGui::Separator();
                }
                ImGui::TreePop();
            }
            ImGui::End();

            for (size_t i = 0; i < sharedResources.ReflectionProbeUniform.Matrices.size(); i++)
            {
                auto& currentProbePosition = sharedResources.ReflectionProbes.Positions[sharedResources.CurrentProbeIndex];
                sharedResources.ReflectionProbeUniform.Matrices[i] = GetReflectionProbeMatrix(currentProbePosition, i);
            }

            renderGraph->Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph->Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.AcquireCurrentSwapchainImage(ImageUsage::TRANSFER_DISTINATION));

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
