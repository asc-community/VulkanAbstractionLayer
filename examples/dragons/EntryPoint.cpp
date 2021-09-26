#include <filesystem>
#include <iostream>

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

struct Mesh
{
    Buffer VertexBuffer;
    Buffer IndexBuffer;
    Buffer InstanceBuffer;
};

struct InstanceData
{
    Vector3 Position;
    uint32_t MaterialIndex;
};

struct MaterialData
{
    uint32_t AlbedoTextureIndex;
    uint32_t NormalTextureIndex;
    float MetallicFactor;
    float RoughnessFactor;
};

constexpr size_t MaxMaterialCount = 256;

struct SharedResources
{
    Buffer CameraUniformBuffer;
    Buffer MeshDataUniformBuffer;
    Buffer LightUniformBuffer;
    Buffer MaterialUniformBuffer;
    std::vector<Mesh> Meshes;
    std::vector<Image> Textures;
    std::vector<MaterialData> Materials;
    Image BRDFLUT;
    Image Skybox;
    Image SkyboxIrradiance;
};

void LoadCubemap(Image& image, const std::string& filepath)
{
    auto& vulkanContext = GetCurrentVulkanContext();
    auto& stageBuffer = vulkanContext.GetCurrentStageBuffer();
    auto& commandBuffer = vulkanContext.GetCurrentCommandBuffer();

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
    
    vulkanContext.SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();
}

void LoadImage(Image& image, const std::string& filepath)
{
    auto& vulkanContext = GetCurrentVulkanContext();
    auto& stageBuffer = vulkanContext.GetCurrentStageBuffer();
    auto& commandBuffer = vulkanContext.GetCurrentCommandBuffer();

    commandBuffer.Begin();

    auto imageData = ImageLoader::LoadImageFromFile(filepath);
    image.Init(
        imageData.Width,
        imageData.Height,
        imageData.ImageFormat,
        ImageUsage::TRANSFER_DISTINATION | ImageUsage::SHADER_READ,
        MemoryUsage::GPU_ONLY,
        ImageOptions::DEFAULT
    );

    auto textureAllocation = stageBuffer.Submit(imageData.ByteData.data(), imageData.ByteData.size());

    commandBuffer.CopyBufferToImage(
        BufferInfo{ stageBuffer.GetBuffer(), textureAllocation.Offset },
        ImageInfo{ image, ImageUsage::UNKNOWN, 0, 0 }
    );

    commandBuffer.TransferLayout(image, ImageUsage::TRANSFER_DISTINATION, ImageUsage::SHADER_READ);

    stageBuffer.Flush();
    commandBuffer.End();

    vulkanContext.SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();
}

Mesh CreateMesh(ArrayView<ModelData::Vertex> vertices, ArrayView<ModelData::Index> indices, ArrayView<InstanceData> instances, ArrayView<ImageData> textures, std::vector<Image>& images)
{
    Mesh result;

    auto& vulkanContext = GetCurrentVulkanContext();
    auto& stageBuffer = vulkanContext.GetCurrentStageBuffer();
    auto& commandBuffer = vulkanContext.GetCurrentCommandBuffer();

    commandBuffer.Begin();

    auto instanceAllocation = stageBuffer.Submit(MakeView(instances));
    auto indexAllocation = stageBuffer.Submit(MakeView(indices));
    auto vertexAllocation = stageBuffer.Submit(MakeView(vertices));

    result.InstanceBuffer.Init(instanceAllocation.Size, BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
    result.IndexBuffer.Init(indexAllocation.Size, BufferUsage::INDEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
    result.VertexBuffer.Init(vertexAllocation.Size, BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);

    commandBuffer.CopyBuffer(
        BufferInfo{ stageBuffer.GetBuffer(), instanceAllocation.Offset }, 
        BufferInfo{ result.InstanceBuffer, 0 }, 
        instanceAllocation.Size
    );
    commandBuffer.CopyBuffer(
        BufferInfo{ stageBuffer.GetBuffer(), indexAllocation.Offset },
        BufferInfo{ result.IndexBuffer, 0 },
        indexAllocation.Size
    );
    commandBuffer.CopyBuffer(
        BufferInfo{ stageBuffer.GetBuffer(), vertexAllocation.Offset }, 
        BufferInfo{ result.VertexBuffer, 0 }, 
        vertexAllocation.Size
    );

    for (const auto& texture : textures)
    {
        auto& image = images.emplace_back();
        image.Init(
            texture.Width,
            texture.Height,
            texture.ImageFormat,
            ImageUsage::TRANSFER_DISTINATION | ImageUsage::TRANSFER_SOURCE | ImageUsage::SHADER_READ,
            MemoryUsage::GPU_ONLY,
            ImageOptions::MIPMAPS
        );

        auto textureAllocation = stageBuffer.Submit(texture.ByteData.data(), texture.ByteData.size());

        commandBuffer.CopyBufferToImage(
            BufferInfo{ stageBuffer.GetBuffer(), textureAllocation.Offset }, 
            ImageInfo{ image, ImageUsage::UNKNOWN, 0, 0 }
        );
        commandBuffer.GenerateMipLevels(image, ImageUsage::TRANSFER_DISTINATION, BlitFilter::LINEAR);
        
        commandBuffer.TransferLayout(image, ImageUsage::TRANSFER_DISTINATION, ImageUsage::SHADER_READ);
    }

    stageBuffer.Flush();
    commandBuffer.End();

    vulkanContext.SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();

    return result;
}

Mesh CreatePlaneMesh(std::vector<MaterialData>& globalMaterials, std::vector<Image>& globalImages)
{
    std::vector vertices = {
        ModelData::Vertex{ { -500.0f, -500.0f, -0.01f }, { -15.0f, -15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ {  500.0f,  500.0f, -0.01f }, {  15.0f,  15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ { -500.0f,  500.0f, -0.01f }, { -15.0f,  15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ {  500.0f,  500.0f, -0.01f }, {  15.0f,  15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ { -500.0f, -500.0f, -0.01f }, { -15.0f, -15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ {  500.0f, -500.0f, -0.01f }, {  15.0f, -15.0f }, { 0.0f, 0.0f, 1.0f } },
    };
    std::vector<ModelData::Index> indices = {
        0, 1, 2, 3, 4, 5
    };
    std::vector instances = {
        InstanceData{ { 0.0f, 0.0f, 0.0f }, (uint32_t)globalMaterials.size() },
    };

    globalMaterials.push_back(MaterialData{
        (uint32_t)globalImages.size() + 0,
        (uint32_t)globalImages.size() + 1,
        0.0f, // metallic
        0.9f, // roughness
    });

    auto albedoTexture = ImageLoader::LoadImageFromFile("../textures/sand_albedo.jpg");
    auto normalTexture = ImageLoader::LoadImageFromFile("../textures/sand_normal.jpg");

    return CreateMesh(vertices, indices, instances, MakeView(std::array{ albedoTexture, normalTexture }), globalImages);
}

Mesh CreateDragonMesh(std::vector<MaterialData>& globalMaterials, std::vector<Image>& globalImages)
{
    std::vector instances = {
        InstanceData{ { 0.0f, 0.0f, -40.0f }, (uint32_t)globalMaterials.size() + 0 },
        InstanceData{ { 0.0f, 0.0f, -20.0f }, (uint32_t)globalMaterials.size() + 1 },
        InstanceData{ { 0.0f, 0.0f,   0.0f }, (uint32_t)globalMaterials.size() + 2 },
        InstanceData{ { 0.0f, 0.0f,  20.0f }, (uint32_t)globalMaterials.size() + 3 },
        InstanceData{ { 0.0f, 0.0f,  40.0f }, (uint32_t)globalMaterials.size() + 4 },
    };

    auto model = ModelLoader::LoadFromObj("../models/dragon/dragon.obj");
    auto& vertices = model.Shapes.front().Vertices;
    auto& indices = model.Shapes.front().Indices;

    std::array albedoTextures = {
        std::vector<uint8_t>{ 255, 255, 255, 255 },
        std::vector<uint8_t>{ 150, 225, 100, 255 },
        std::vector<uint8_t>{ 100, 150, 225, 255 },
        std::vector<uint8_t>{ 255, 220,  60, 255 },
        std::vector<uint8_t>{ 150, 150, 150, 255 },
    };

    std::array normalTextures = {
        std::vector < uint8_t>{ 127, 127, 255, 255 }
    };

    std::array textures = {
        ImageData{ std::move(normalTextures[0]), Format::R8G8B8A8_UNORM, 1, 1 },
        ImageData{ std::move(albedoTextures[0]), Format::R8G8B8A8_UNORM, 1, 1 },
        ImageData{ std::move(albedoTextures[1]), Format::R8G8B8A8_UNORM, 1, 1 },
        ImageData{ std::move(albedoTextures[2]), Format::R8G8B8A8_UNORM, 1, 1 },
        ImageData{ std::move(albedoTextures[3]), Format::R8G8B8A8_UNORM, 1, 1 },
        ImageData{ std::move(albedoTextures[4]), Format::R8G8B8A8_UNORM, 1, 1 },
    };

    globalMaterials.push_back(MaterialData{
        (uint32_t)globalImages.size() + 1,
        (uint32_t)globalImages.size(), // default normal
        0.0f, // metallic
        1.0f, // roughness
    });
    globalMaterials.push_back(MaterialData{
        (uint32_t)globalImages.size() + 2,
        (uint32_t)globalImages.size(), // default normal
        1.0f, // metallic
        0.7f, // roughness
    });
    globalMaterials.push_back(MaterialData{
        (uint32_t)globalImages.size() + 3,
        (uint32_t)globalImages.size(), // default normal
        0.0f, // metallic
        0.0f, // roughness
    });
    globalMaterials.push_back(MaterialData{
        (uint32_t)globalImages.size() + 4,
        (uint32_t)globalImages.size(), // default normal
        1.0f, // metallic
        0.0f, // roughness
    });
    globalMaterials.push_back(MaterialData{
        (uint32_t)globalImages.size() + 5,
        (uint32_t)globalImages.size(), // default normal
        0.8f, // metallic
        0.5f, // roughness
    });

    return CreateMesh(vertices, indices, instances, textures, globalImages);
}

class UniformSubmitRenderPass : public RenderPass
{
    SharedResources& sharedResources;

public:
    struct CameraUniformData
    {
        Matrix4x4 Matrix;
        Vector3 Position;
    } CameraUniform;

    struct ModelUniformData
    {
        Matrix3x4 Matrix;
    } ModelUniform;

    struct LightUniformData
    {
        Matrix4x4 Projection;
        Vector3 Color;
        float AmbientIntensity;
        Vector3 Direction;
    } LightUniform;

    UniformSubmitRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.AddDependency("CameraUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
        pipeline.AddDependency("MeshDataUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
        pipeline.AddDependency("LightUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
        pipeline.AddDependency("MaterialUniformBuffer", BufferUsage::TRANSFER_DESTINATION);
    }

    virtual void ResolveResources(ResolveState resolve) override
    {
        resolve.Resolve("CameraUniformBuffer", this->sharedResources.CameraUniformBuffer);
        resolve.Resolve("MeshDataUniformBuffer", this->sharedResources.MeshDataUniformBuffer);
        resolve.Resolve("LightUniformBuffer", this->sharedResources.LightUniformBuffer);
        resolve.Resolve("MaterialUniformBuffer", this->sharedResources.MaterialUniformBuffer);
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

        auto FillUniformArray = [&state](const auto& uniformData, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(MakeView(uniformData));
            state.Commands.CopyBuffer(
                BufferInfo{ stageBuffer.GetBuffer(), uniformAllocation.Offset },
                BufferInfo{ uniformBuffer, 0 },
                uniformAllocation.Size
            );
        };

        FillUniform(this->CameraUniform, this->sharedResources.CameraUniformBuffer);
        FillUniform(this->ModelUniform, this->sharedResources.MeshDataUniformBuffer);
        FillUniform(this->LightUniform, this->sharedResources.LightUniformBuffer);
        FillUniformArray(this->sharedResources.Materials, this->sharedResources.MaterialUniformBuffer);
    }
};

class ShadowRenderPass : public RenderPass
{
    SharedResources& sharedResources;
public:
    ShadowRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("shadow_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("shadow_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );
        
        pipeline.DeclareAttachment("ShadowDepth", Format::D32_SFLOAT_S8_UINT, 2048, 2048);

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                5,
            },
            VertexBinding{
                VertexBinding::Rate::PER_INSTANCE,
                2,
            },
        };

        pipeline.DescriptorBindings
            .Bind(1, "MeshDataUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(2, "LightUniformBuffer", UniformType::UNIFORM_BUFFER);

        pipeline.AddOutputAttachment("ShadowDepth", ClearDepthStencil{ });
    }

    virtual void ResolveResources(ResolveState resolve) override
    {

    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("ShadowDepth");
        state.Commands.SetRenderArea(output);

        for (const auto& mesh : this->sharedResources.Meshes)
        {
            size_t indexCount = mesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);
            size_t instanceCount = mesh.InstanceBuffer.GetByteSize() / sizeof(InstanceData);
            state.Commands.BindVertexBuffers(mesh.VertexBuffer, mesh.InstanceBuffer);
            state.Commands.BindIndexBufferUInt32(mesh.IndexBuffer);
            state.Commands.DrawIndexed((uint32_t)indexCount, (uint32_t)instanceCount);
        }
    }
};

class OpaqueRenderPass : public RenderPass
{    
    SharedResources& sharedResources;
    Sampler textureSampler;
    Sampler depthSampler;
public:
    OpaqueRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->textureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);
        this->depthSampler.Init(Sampler::MinFilter::NEAREST, Sampler::MagFilter::NEAREST, Sampler::AddressMode::CLAMP_TO_EDGE, Sampler::MipFilter::NEAREST);
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                5,
            },
            VertexBinding{
                VertexBinding::Rate::PER_INSTANCE,
                2,
            },
        };

        pipeline.DeclareAttachment("Output", Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth", Format::D32_SFLOAT_S8_UINT);

        pipeline.DescriptorBindings
            .Bind(0, "CameraUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(1, "MeshDataUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(2, "LightUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(3, "MaterialUniformBuffer", UniformType::UNIFORM_BUFFER)
            .Bind(4, this->textureSampler, UniformType::SAMPLER)
            .Bind(5, "Textures", UniformType::SAMPLED_IMAGE)
            .Bind(6, "ShadowDepth", this->depthSampler, UniformType::COMBINED_IMAGE_SAMPLER, ImageView::DEPTH_ONLY)
            .Bind(7, "BRDFLUT", this->textureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(8, "Skybox", this->textureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(9, "SkyboxIrradiance", this->textureSampler, UniformType::COMBINED_IMAGE_SAMPLER);

        pipeline.AddOutputAttachment("Output", ClearColor{ 0.5f, 0.8f, 1.0f, 1.0f });
        pipeline.AddOutputAttachment("OutputDepth", ClearDepthStencil{ });
    }

    virtual void ResolveResources(ResolveState resolve) override
    {
        resolve.Resolve("Textures", this->sharedResources.Textures);
        resolve.Resolve("BRDFLUT", this->sharedResources.BRDFLUT);
        resolve.Resolve("Skybox", this->sharedResources.Skybox);
        resolve.Resolve("SkyboxIrradiance", this->sharedResources.SkyboxIrradiance);
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output");
        state.Commands.SetRenderArea(output);

        for (const auto& mesh : this->sharedResources.Meshes)
        {
            size_t indexCount = mesh.IndexBuffer.GetByteSize() / sizeof(ModelData::Index);
            size_t instanceCount = mesh.InstanceBuffer.GetByteSize() / sizeof(InstanceData);
            state.Commands.BindVertexBuffers(mesh.VertexBuffer, mesh.InstanceBuffer);
            state.Commands.BindIndexBufferUInt32(mesh.IndexBuffer);
            state.Commands.DrawIndexed((uint32_t)indexCount, (uint32_t)instanceCount);
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
            .Bind(8, "Skybox", this->skyboxSampler, UniformType::COMBINED_IMAGE_SAMPLER);

        pipeline.AddOutputAttachment("Output", AttachmentState::LOAD_COLOR);
        pipeline.AddOutputAttachment("OutputDepth", AttachmentState::LOAD_DEPTH_SPENCIL);
    }

    virtual void ResolveResources(ResolveState resolve) override
    {

    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output");
        state.Commands.SetRenderArea(output);

        constexpr uint32_t SkyboxVertexCount = 36;
        state.Commands.Draw(SkyboxVertexCount, 1);
    }
};

auto CreateRenderGraph(SharedResources& resources)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass", std::make_unique<UniformSubmitRenderPass>(resources))
        .AddRenderPass("ShadowPass", std::make_unique<ShadowRenderPass>(resources))
        .AddRenderPass("OpaquePass", std::make_unique<OpaqueRenderPass>(resources))
        .AddRenderPass("SkyboxPass", std::make_unique<SkyboxRenderPass>(resources))
        .AddRenderPass("ImGuiPass", std::make_unique<ImGuiRenderPass>("Output"))
        .SetOutputName("Output");

    return renderGraphBuilder.Build();
}

struct Camera
{
    Vector3 Position{ 40.0f, 25.0f, -90.0f };
    Vector2 Rotation{ 5.74f, 0.0f };
    float Fov = 65.0f;
    float MovementSpeed = 250.0f;
    float RotationMovementSpeed = 2.5f;
    float AspectRatio = 16.0f / 9.0f;
    float ZNear = 0.1f;
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

int main()
{
    if(std::filesystem::exists(APPLICATION_WORKING_DIRECTORY))
        std::filesystem::current_path(APPLICATION_WORKING_DIRECTORY);

    WindowCreateOptions windowOptions;
    windowOptions.Position = { 300.0f, 100.0f };
    windowOptions.Size = { 1280.0f, 720.0f };
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
        Buffer{ sizeof(UniformSubmitRenderPass::CameraUniform), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(UniformSubmitRenderPass::ModelUniform),  BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(UniformSubmitRenderPass::LightUniform),  BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(MaterialData) * MaxMaterialCount,        BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { }, // meshes
        { }, // mesh textures
        { }, // materials
        Image{ }, // brdf lut
        Image{ }, // skybox
        Image{ }, // skybox irradiance
    };

    LoadImage(sharedResources.BRDFLUT, "../textures/brdf_lut.dds");
    LoadCubemap(sharedResources.Skybox, "../textures/skybox.png");
    LoadCubemap(sharedResources.SkyboxIrradiance, "../textures/skybox_irradiance.png");

    sharedResources.Meshes.push_back(CreatePlaneMesh(sharedResources.Materials, sharedResources.Textures));
    sharedResources.Meshes.push_back(CreateDragonMesh(sharedResources.Materials, sharedResources.Textures));

    std::unique_ptr<RenderGraph> renderGraph = CreateRenderGraph(sharedResources);

    Camera camera;
    Vector3 modelRotation{ -HalfPi, Pi, 0.0f };
    Vector3 lightColor{ 0.7f, 0.7f, 0.7f };
    Vector3 lightDirection{ -0.3f, 1.0f, -0.6f };
    float lightBounds = 50.0f;
    float lightAmbientIntensity = 0.7f;

    window.OnResize([&Vulkan, &sharedResources, &renderGraph, &camera](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(sharedResources);
        camera.AspectRatio = size.x / size.y;
    });
    
    ImGuiVulkanContext::Init(window, renderGraph->GetNodeByName("ImGuiPass").PassNative.RenderPassHandle);

    std::unordered_map<uint32_t, ImTextureID> imguiMappings;
    Sampler imguiSampler = Sampler{ Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::CLAMP_TO_EDGE, Sampler::MipFilter::LINEAR };
    for (const auto& material : sharedResources.Materials)
    {
        if (imguiMappings.find(material.AlbedoTextureIndex) == imguiMappings.end())
        {
            imguiMappings.emplace(
                material.AlbedoTextureIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Textures[material.AlbedoTextureIndex], imguiSampler)
            );
        }
        if (imguiMappings.find(material.NormalTextureIndex) == imguiMappings.end())
        {
            imguiMappings.emplace(
                material.NormalTextureIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Textures[material.NormalTextureIndex], imguiSampler)
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

            auto mouseMovement = ImGui::GetMouseDragDelta(MouseButton::RIGHT, 0.0f);
            ImGui::ResetMouseDragDelta(MouseButton::RIGHT);
            camera.Rotate(Vector2{ -mouseMovement.x, -mouseMovement.y } * dt);

            Vector3 movementDirection{ 0.0f };
            if (ImGui::IsKeyDown(KeyCode::W))
                movementDirection += Vector3{  1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::A))
                movementDirection += Vector3{  0.0f,  0.0f, -1.0f };
            if (ImGui::IsKeyDown(KeyCode::S))
                movementDirection += Vector3{ -1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::D))
                movementDirection += Vector3{  0.0f,  0.0f,  1.0f };
            if (ImGui::IsKeyDown(KeyCode::SPACE))
                movementDirection += Vector3{  0.0f,  1.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::LEFT_SHIFT))
                movementDirection += Vector3{  0.0f, -1.0f,  0.0f };
            if (movementDirection != Vector3{ 0.0f }) movementDirection = Normalize(movementDirection);
            camera.Move(movementDirection * dt);

            ImGui::Begin("Camera");
            ImGui::DragFloat("movement speed", &camera.MovementSpeed, 0.1f);
            ImGui::DragFloat("rotation movement speed", &camera.RotationMovementSpeed, 0.1f);
            ImGui::DragFloat3("position", &camera.Position[0]);
            ImGui::DragFloat2("rotation", &camera.Rotation[0], 0.01f);
            ImGui::DragFloat("fov", &camera.Fov);
            ImGui::End();

            ImGui::Begin("Model");
            ImGui::DragFloat3("rotation", &modelRotation[0], 0.01f);
            ImGui::End();

            ImGui::Begin("Light");
            ImGui::ColorEdit3("color", &lightColor[0], ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
            ImGui::DragFloat3("direction", &lightDirection[0], 0.01f);
            ImGui::DragFloat("bounds", &lightBounds, 0.1f);
            ImGui::DragFloat("ambient intensity", &lightAmbientIntensity, 0.01f, 0.0f, 1.0f);
            ImGui::End();

            int materialIndex = 0;
            ImGui::Begin("Materials");
            for (auto& material : sharedResources.Materials)
            {
                ImGui::PushID(materialIndex++);

                ImGui::BeginTable(("material_" + std::to_string(materialIndex)).c_str(), 3);

                ImGui::TableSetupColumn("parameters");
                ImGui::TableSetupColumn("albedo image");
                ImGui::TableSetupColumn("normal image");
                ImGui::TableHeadersRow();

                ImGui::TableNextColumn();
                ImGui::DragFloat("metallic", &material.MetallicFactor, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("roughness", &material.RoughnessFactor, 0.01f, 0.0f, 1.0f);
                ImGui::TableNextColumn();
                ImGui::Image(imguiMappings.at(material.AlbedoTextureIndex), { 128.0f, 128.0f });
                ImGui::TableNextColumn();
                ImGui::Image(imguiMappings.at(material.NormalTextureIndex), { 128.0f, 128.0f });

                ImGui::EndTable();

                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::End();

            ImGui::Begin("Performace");
            ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
            ImGui::End();

            Vector3 low { -lightBounds, -lightBounds, -lightBounds };
            Vector3 high{ lightBounds, lightBounds, lightBounds };

            auto& uniformSubmitPass = renderGraph->GetRenderPassByName<UniformSubmitRenderPass>("UniformSubmitPass");
            uniformSubmitPass.CameraUniform.Matrix = camera.GetMatrix();
            uniformSubmitPass.CameraUniform.Position = camera.Position;
            uniformSubmitPass.ModelUniform.Matrix = MakeRotationMatrix(modelRotation);
            uniformSubmitPass.LightUniform.Color = lightColor;
            uniformSubmitPass.LightUniform.AmbientIntensity = lightAmbientIntensity;
            uniformSubmitPass.LightUniform.Direction = Normalize(lightDirection);
            uniformSubmitPass.LightUniform.Projection =
                MakeOrthographicMatrix(low.x, high.x, low.y, high.y, low.z, high.z) *
                MakeLookAtMatrix(Vector3{ 0.0f, 0.0f, 0.0f }, -lightDirection, Vector3{ 0.001f, 1.0f, 0.001f });

            renderGraph->Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph->Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.AcquireCurrentSwapchainImage(ImageUsage::TRANSFER_DISTINATION));

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
