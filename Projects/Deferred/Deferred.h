#pragma once

#include "VK/VkBase.h"

#include <string>
#include <vector>

#include "VK/Buffer.h"
#include "VK/Framebuffer.h"
#include "VK/Model.h"
#include "VK/Texture.h"
#include "View/Camera.h"

class Deferred : public VkBase {
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
  void UpdateOffscreenUniformBuffers();
  void UpdateCompositionUniformBuffers();

  void SetupDescriptorSetLayout();
  void SetupPipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();

  void BuildCommandBuffers() override;

  void BuildDeferredCommandBuffer();

  void ViewChanged() override;

private:
  VertexLayout vertexLayout{
      {
          VertexLayoutComponent::Position,
          VertexLayoutComponent::Color,
          VertexLayoutComponent::Normal,
      },
  };

  struct {
    Model teapot;
    Model torus;
    Model floor;
  } models;

  struct {
    alignas(16) glm::mat4 viewProj;
  } uboOffscreenVS;

  struct Light {
    alignas(16) glm::vec4 pos;
    alignas(16) glm::vec3 color;
    alignas(4) float radius;
  };

  struct {
    Light lights[8];
    alignas(16) glm::vec4 viewPos;
    alignas(4) int lightsNum;
    alignas(4) int dispTarget;
  } uboComposition;

  struct {
    Buffer offscreen;
    Buffer composition;
  } uniformBuffers;

  struct {
    VkPipeline offscreen;
    VkPipeline composition;
  } pipelines;
  VkPipelineLayout pipelineLayout;

  struct {
    VkDescriptorSet offscreen;
    VkDescriptorSet composition;
  } descriptorSets;
  VkDescriptorSetLayout descriptorSetLayout;

  Framebuffer offscreenFramebuffer;

  VkCommandBuffer offscreenCmdBuffer = VK_NULL_HANDLE;
  VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;

  Camera camera{};

  float prevTime = 0.0f;
  float camAngle = 0.0f;

  struct Settings {
    int dispRenderTarget = 0;
  } settings;
};
