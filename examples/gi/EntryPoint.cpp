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

struct Mesh
{
    struct Material
    {
        uint32_t AlbedoIndex;
        uint32_t NormalIndex;
        uint32_t MetallicRoughnessIndex;
        float RoughnessScale;
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
};

struct CameraUniformData
{
    Matrix4x4 Matrix;
    Vector3 Position;
};

struct ModelUniformData
{
    Matrix3x4 Matrix;
};

struct ReflectionProbe
{
    Vector3 Position;
    Image Cubemap;
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

struct SharedResources
{
    Buffer CameraUniformBuffer;
    Buffer ModelUniformBuffer;
    Buffer MaterialUniformBuffer;
    Buffer ReflectionProbeUniformBuffer;
    Mesh Sponza;
    Mesh Sphere;
    CameraUniformData CameraUniform;
    ModelUniformData ModelUniform;
    ReflectionProbeUniformData ReflectionProbeUniform;
    std::vector<ReflectionProbe> ReflectionProbes;
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

        mesh.Materials.push_back(Mesh::Material{ textureIndex, textureIndex + 1, textureIndex + 2, material.RoughnessScale });
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

public:
    UniformSubmitRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.DeclareBuffer(this->sharedResources.CameraUniformBuffer);
        pipeline.DeclareBuffer(this->sharedResources.ModelUniformBuffer);
        pipeline.DeclareBuffer(this->sharedResources.MaterialUniformBuffer);
        pipeline.DeclareBuffer(this->sharedResources.ReflectionProbeUniformBuffer);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddBuffer(this->sharedResources.CameraUniformBuffer,          BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.ModelUniformBuffer,           BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.MaterialUniformBuffer,        BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.ReflectionProbeUniformBuffer, BufferUsage::TRANSFER_DESTINATION);
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

        FillUniform(this->sharedResources.CameraUniform, this->sharedResources.CameraUniformBuffer);
        FillUniform(this->sharedResources.ModelUniform, this->sharedResources.ModelUniformBuffer);
        FillUniformArray(this->sharedResources.Sponza.Materials, this->sharedResources.MaterialUniformBuffer);
        FillUniform(this->sharedResources.ReflectionProbeUniform, this->sharedResources.ReflectionProbeUniformBuffer);
    }
};

class ReflectionCalculateRenderPass : public RenderPass
{
    SharedResources& sharedResources;
    std::vector<ImageReference> textureArray;
    Sampler TextureSampler;
public:

    ReflectionCalculateRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->TextureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

        for (const auto& texture : this->sharedResources.Sponza.Textures)
        {
            this->textureArray.push_back(std::ref(texture));
        }
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("probe_view_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                VertexBinding::BindingRangeAll
            }
        };

        constexpr uint32_t probeResolution = 1024;
        pipeline.DeclareAttachment("OutputProbe"_id, Format::R8G8B8A8_UNORM, probeResolution, probeResolution, ImageOptions::CUBEMAP);
        pipeline.DeclareAttachment("OutputProbeDepth"_id, Format::D32_SFLOAT_S8_UINT, probeResolution, probeResolution, ImageOptions::CUBEMAP);

        pipeline.DeclareImages(this->textureArray, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareImage(this->sharedResources.BRDFLUT, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareImage(this->sharedResources.Skybox, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareImage(this->sharedResources.SkyboxIrradiance, ImageUsage::TRANSFER_DISTINATION);

        pipeline.DescriptorBindings
            .Bind(0, this->sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, this->sharedResources.ReflectionProbeUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(2, this->sharedResources.ModelUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(3, this->sharedResources.MaterialUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(4, this->textureArray, UniformType::SAMPLED_IMAGE)
            .Bind(5, this->TextureSampler, UniformType::SAMPLER)
            .Bind(6, this->sharedResources.BRDFLUT, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(7, this->sharedResources.Skybox, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(8, this->sharedResources.SkyboxIrradiance, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER);

        pipeline.AddOutputAttachment("OutputProbe"_id, ClearColor{ 0.05f, 0.0f, 0.1f, 1.0f });
        pipeline.AddOutputAttachment("OutputProbeDepth"_id, ClearDepthStencil{ });
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("OutputProbe"_id);
        state.Commands.SetRenderArea(output);

        struct
        {
            Vector3 CameraPosition;
            uint32_t MaterialIndex;
        } pushConstants;

        for (const auto& submesh : this->sharedResources.Sponza.Submeshes)
        {
            pushConstants.CameraPosition = sharedResources.ReflectionProbes[0].Position;
            pushConstants.MaterialIndex = submesh.MaterialIndex;

            size_t indexCount = submesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);
            state.Commands.PushConstants(state.Pass, &pushConstants);
            state.Commands.BindVertexBuffers(submesh.VertexBuffer);
            state.Commands.BindIndexBufferUInt32(submesh.IndexBuffer);
            state.Commands.DrawIndexed((uint32_t)indexCount, 1);
        }
    }
};

class OpaqueRenderPass : public RenderPass
{    
    SharedResources& sharedResources;
    std::vector<ImageReference> textureArray;
public:
    Sampler TextureSampler;

    OpaqueRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->TextureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

        for (const auto& texture : this->sharedResources.Sponza.Textures)
        {
            this->textureArray.push_back(std::ref(texture));
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

        pipeline.DeclareAttachment("Output"_id, Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT);

        pipeline.DescriptorBindings
            .Bind(0, this->sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, this->sharedResources.ReflectionProbeUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(2, this->sharedResources.ModelUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(3, this->sharedResources.MaterialUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(4, this->textureArray, UniformType::SAMPLED_IMAGE)
            .Bind(5, this->TextureSampler, UniformType::SAMPLER)
            .Bind(6, this->sharedResources.BRDFLUT, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(7, this->sharedResources.Skybox, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(8, this->sharedResources.SkyboxIrradiance, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER);

        pipeline.AddOutputAttachment("Output"_id, ClearColor{ 0.05f, 0.0f, 0.1f, 1.0f });
        pipeline.AddOutputAttachment("OutputDepth"_id, ClearDepthStencil{ });
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output"_id);
        state.Commands.SetRenderArea(output);

        struct
        {
            Vector3 CameraPosition;
            uint32_t MaterialIndex;
        } pushConstants;

        for (const auto& submesh : this->sharedResources.Sponza.Submeshes)
        {
            pushConstants.CameraPosition = sharedResources.CameraUniform.Position;
            pushConstants.MaterialIndex = submesh.MaterialIndex;

            size_t indexCount = submesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);
            state.Commands.PushConstants(state.Pass, &pushConstants);
            state.Commands.BindVertexBuffers(submesh.VertexBuffer);
            state.Commands.BindIndexBufferUInt32(submesh.IndexBuffer);
            state.Commands.DrawIndexed((uint32_t)indexCount, 1);
        }
    }
};

class ReflectionProbeRenderPass : public RenderPass
{
    SharedResources& sharedResources;
    std::vector<ImageReference> reflectionProbes;
    Sampler textureSampler;
public:
    ReflectionProbeRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->textureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);
        for (const auto& probe : this->sharedResources.ReflectionProbes)
            reflectionProbes.push_back(std::ref(probe.Cubemap));
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

        pipeline.DeclareImages(this->reflectionProbes, ImageUsage::TRANSFER_DISTINATION);

        pipeline.DescriptorBindings
            .Bind(0, this->sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, "OutputProbe"_id, UniformType::SAMPLED_IMAGE, ImageView::NATIVE)
            .Bind(2, this->textureSampler, UniformType::SAMPLER);

        pipeline.AddOutputAttachment("Output"_id, AttachmentState::LOAD_COLOR);
        pipeline.AddOutputAttachment("OutputDepth"_id, AttachmentState::LOAD_DEPTH_SPENCIL);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output"_id);
        state.Commands.SetRenderArea(output);

        struct
        {
            Vector3 Position;
            float Size;
            uint32_t ProbeIndex;
        } pushConstants;

        uint32_t probeIndex = 0;
        for (const auto& probe : this->sharedResources.ReflectionProbes)
        {
            const auto& sphereMesh = this->sharedResources.Sphere.Submeshes[0];
            size_t indexCount = sphereMesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);

            pushConstants.Position = probe.Position;
            pushConstants.Size = 20.0f;
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
            .Bind(0, this->sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(8, this->sharedResources.Skybox, this->skyboxSampler, UniformType::COMBINED_IMAGE_SAMPLER);

        pipeline.AddOutputAttachment("Output"_id, AttachmentState::LOAD_COLOR);
        pipeline.AddOutputAttachment("OutputDepth"_id, AttachmentState::LOAD_DEPTH_SPENCIL);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output"_id);
        state.Commands.SetRenderArea(output);

        constexpr uint32_t SkyboxVertexCount = 36;
        state.Commands.Draw(SkyboxVertexCount, 1);
    }
};

auto CreateRenderGraph(SharedResources& resources, RenderGraphOptions::Value options)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass"_id, std::make_unique<UniformSubmitRenderPass>(resources))
        .AddRenderPass("ReflectionCalculatePass"_id, std::make_unique<ReflectionCalculateRenderPass>(resources))
        .AddRenderPass("OpaquePass"_id, std::make_unique<OpaqueRenderPass>(resources))
        .AddRenderPass("ReflectionProbePass"_id, std::make_unique<ReflectionProbeRenderPass>(resources))
        .AddRenderPass("SkyboxPass"_id, std::make_unique<SkyboxRenderPass>(resources))
        .AddRenderPass("ImGuiPass"_id, std::make_unique<ImGuiRenderPass>("Output"_id))
        .SetOptions(options)
        .SetOutputName("Output"_id);

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
        Buffer{ sizeof(ModelUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(Mesh::Material) * MaxMaterialCount, BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(ReflectionProbeUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { }, // sponza
        { }, // sphere
        { }, // camera uniform
        { }, // model uniform
        { }, // reflection probe uniform
        { }, // reflection probes
        { }, // skybox
        { }, // skybox irradiance
        { }, // brdf lut
        std::make_shared<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        ),
    };

    auto& testProbe = sharedResources.ReflectionProbes.emplace_back();
    testProbe.Position = Vector3{ -50.0f, 150.0f, 200.0f };
    LoadCubemap(testProbe.Cubemap, "../textures/skybox.png");

    LoadModel(sharedResources.Sphere, "../models/sphere/sphere.obj");
    LoadModel(sharedResources.Sponza, "../models/Sponza/glTF/Sponza.gltf");

    LoadCubemap(sharedResources.Skybox, "../textures/skybox.png");
    LoadCubemap(sharedResources.SkyboxIrradiance, "../textures/skybox_irradiance.png");
    LoadImage(sharedResources.BRDFLUT, "../textures/brdf_lut.dds", ImageOptions::DEFAULT);

    Sampler ImGuiImageSampler(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

    std::unique_ptr<RenderGraph> renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::Value{ });

    Camera camera;
    Vector3 modelRotation{ 0.0f, HalfPi, 0.0f };

    window.OnResize([&Vulkan, &sharedResources, &renderGraph, &camera](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::ON_SWAPCHAIN_RESIZE);
        camera.AspectRatio = size.x / size.y;
    });
    
    ImGuiVulkanContext::Init(window, renderGraph->GetNodeByName("ImGuiPass"_id).PassNative.RenderPassHandle);

    std::map<size_t, ImTextureID> ImGuiRegisteredImages;
    for (const auto& material : sharedResources.Sponza.Materials)
    {
        if (ImGuiRegisteredImages.find(material.AlbedoIndex) == ImGuiRegisteredImages.end())
            ImGuiRegisteredImages.emplace(
                material.AlbedoIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Sponza.Textures[material.AlbedoIndex], ImGuiImageSampler)
            );
        if (ImGuiRegisteredImages.find(material.NormalIndex) == ImGuiRegisteredImages.end())
            ImGuiRegisteredImages.emplace(
                material.NormalIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Sponza.Textures[material.NormalIndex], ImGuiImageSampler)
            );
        if (ImGuiRegisteredImages.find(material.MetallicRoughnessIndex) == ImGuiRegisteredImages.end())
            ImGuiRegisteredImages.emplace(
                material.MetallicRoughnessIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Sponza.Textures[material.MetallicRoughnessIndex], ImGuiImageSampler)
            );
    }

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (Vulkan.IsRenderingEnabled())
        {
            Vulkan.StartFrame();
            ImGuiVulkanContext::StartFrame();

            auto dt = ImGui::GetIO().DeltaTime;

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


            ImGui::Begin("Model");
            ImGui::DragFloat3("rotation", &modelRotation[0], 0.01f);
            ImGui::End();

            sharedResources.ModelUniform.Matrix = MakeRotationMatrix(modelRotation);

            ImGui::Begin("Performace");
            ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
            ImGui::End();

            int materialIndex = 0;
            ImGui::Begin("Sponza materials");
            for (auto& material : sharedResources.Sponza.Materials)
            {
                ImGui::PushID(materialIndex++);

                ImGui::BeginTable(("material_" + std::to_string(materialIndex)).c_str(), 4);

                ImGui::TableSetupColumn("roughness");
                ImGui::TableSetupColumn("albedo image");
                ImGui::TableSetupColumn("normal image");
                ImGui::TableSetupColumn("metallic-roughness image");
                ImGui::TableHeadersRow();

                ImGui::TableNextColumn();
                ImGui::DragFloat("scale", &material.RoughnessScale, 0.01f, 0.0f, 1.0f);
                ImGui::TableNextColumn();
                ImGui::Image(ImGuiRegisteredImages.at(material.AlbedoIndex), { 128.0f, 128.0f });
                ImGui::TableNextColumn();
                ImGui::Image(ImGuiRegisteredImages.at(material.NormalIndex), { 128.0f, 128.0f });
                ImGui::TableNextColumn();
                ImGui::Image(ImGuiRegisteredImages.at(material.MetallicRoughnessIndex), { 128.0f, 128.0f });

                ImGui::EndTable();

                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::End();

            int reflectionProbeIndex = 0;
            ImGui::Begin("Reflection probes");
            ImGui::PushID(reflectionProbeIndex++);

            for (auto& probe : sharedResources.ReflectionProbes)
            {
                ImGui::DragFloat3("position", &probe.Position[0]);

                for (size_t i = 0; i < sharedResources.ReflectionProbeUniform.Matrices.size(); i++)
                {
                    sharedResources.ReflectionProbeUniform.Matrices[i] = GetReflectionProbeMatrix(probe.Position, i);
                }
            }

            ImGui::Separator();
            ImGui::PopID();
            ImGui::End();

            renderGraph->Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph->Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.AcquireCurrentSwapchainImage(ImageUsage::TRANSFER_DISTINATION));

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
