#pragma once

#include "VK/VkBase.h"

#include <string>
#include <vector>

#include "VK/Buffer.h"
#include "VK/Framebuffer.h"
#include "VK/Model.h"
#include "VK/Texture.h"
#include "View/Camera.h"

class SSAO : public VkBase {
public:
  void OnPostInit() override;
  void OnPreDestroy() override;
  void OnRender() override;
  void OnUpdate(float t) override;
  void OnUpdateUIOverlay() override;

  void LoadAssets();
  void PrepareOffscreenFramebuffer();
  void PrepareUniformBuffers();

  void UpdateUniformBuffers();
  void UpdateSceneUniformBuffers();
  void UpdateSSAOUniformBuffers();

  void SetupDescriptorPool();
  void SetupDescriptorSet();
  void SetupPipelines();

  void BuildCommandBuffers() override;

  void ViewChanged() override;

private:
  static constexpr inline size_t KERNEL_SIZE = 64;
  static constexpr inline size_t ROT_TEX_SIZE = 4;

  VertexLayout vertexLayout{
      {
          VertexLayoutComponent::Position,
          VertexLayoutComponent::Normal,
          VertexLayoutComponent::Color,
          VertexLayoutComponent::UV,
      },
  };

  struct {
    Model teapot;
    Model floor;
  } models;

  struct {
    Texture2D noise;
  } textures;

  struct {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(4) float nearPlane;
    alignas(4) float farPlane;
  } uboScene;

  struct {
    alignas(16) glm::vec4 kernel[KERNEL_SIZE];
    alignas(4) float radius;
    alignas(4) float bias;
  } uboKernel;

  struct Light {
    alignas(16) glm::vec4 pos;
    alignas(16) glm::vec3 diff;
    alignas(16) glm::vec3 amb;
  };

  struct {
    alignas(16) glm::mat4 proj;
    Light light;
    alignas(4) int displayRenderTarget;
    alignas(4) bool useBlur;
    alignas(4) float ao;
  } uboSSAO;

  struct {
    Buffer scene;
    Buffer ssaoKernel;
    Buffer ssao;
  } uniformBuffers;

  struct {
    VkPipeline gBuffer;
    VkPipeline ssao;
    VkPipeline blur;
    VkPipeline lighting;
  } pipelines;

  struct {
    VkPipelineLayout gBuffer;
    VkPipelineLayout ssao;
    VkPipelineLayout blur;
    VkPipelineLayout lighting;
  } pipelineLayouts;

  struct {
    VkDescriptorSet gBuffer;
    VkDescriptorSet ssao;
    VkDescriptorSet blur;
    VkDescriptorSet lighting;
    const uint32_t count = 5;
  } descriptorSets;

  struct {
    VkDescriptorSetLayout gBuffer;
    VkDescriptorSetLayout ssao;
    VkDescriptorSetLayout blur;
    VkDescriptorSetLayout lighting;
  } descriptorSetLayouts;

  struct {
    Framebuffer gBuffer;
    Framebuffer ssao;
    Framebuffer blur;
  } frameBuffers;

  VkSampler colorSampler = VK_NULL_HANDLE;

  VkCommandBuffer offscreenCmdBuffer = VK_NULL_HANDLE;
  VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;

  Camera camera{};

  float prevTime = 0.0f;
  float camAngle = 0.0f;
};
