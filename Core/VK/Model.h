/**
 * #brief Assimpを使ったモデルローダ
 */

#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/postprocess.h>

#include <map>
#include <string>
#include <vector>

#include "VK/Buffer.h"
#include "VK/Device.h"

enum struct VertexLayoutComponent {
  Position = 0x00,
  Normal = 0x01,
  Color = 0x02,
  UV = 0x03,
  Tangent = 0x04,
  Bitangent = 0x05,
  DummyFloat = 0x06,
  DummyVec4 = 0x07,
};

/**
 *  モデルのロードと頂点入力および属性バインディング用の頂点レイアウトコンポーネントを格納します。
 */
struct VertexLayout {
  explicit VertexLayout(std::vector<VertexLayoutComponent> &&vertexLayoutComponents)
      : components(std::move(vertexLayoutComponents)) {}

  [[nodiscard]] uint32_t Stride() const {
    static const std::map<VertexLayoutComponent, uint32_t> c2s = {
        {VertexLayoutComponent::UV, 2 * sizeof(float)},
        {VertexLayoutComponent::DummyFloat, sizeof(float)},
        {VertexLayoutComponent::DummyVec4, 4 * sizeof(float)},
    };

    uint32_t res = 0;
    for (const auto &component : components) {
      if (c2s.contains(component)) {
        res += c2s.at(component);
      } else {
        res += 3 * sizeof(float);
      }
    }
    return res;
  }
  std::vector<VertexLayoutComponent> components;
};

struct ModelCreateInfo {
  glm::vec3 center = glm::vec3(0.0f);
  glm::vec3 scale = glm::vec3(1.0f);
  glm::vec2 uvscale = glm::vec2(1.0f);
  VkMemoryPropertyFlags memoryPropertyFlags = 0;
};

struct Model {
  bool LoadFromFile(const Device &device,
                                  const std::string &filepath,
                                  VkQueue copyQueue,
                                  const VertexLayout &vertexLayout,
                                  const ModelCreateInfo &modelCreateInfo = {});
  void Destroy(const Device &device) const;

  Buffer vertices{};
  uint32_t vertexCount = 0;
  Buffer indices{};
  uint32_t indexCount = 0;
  /**
   * @brief モデルの各部分の頂点とインデックスのベースとカウントを格納します。
   */
  struct Mesh {
    uint32_t vertexBase = 0;
    uint32_t vertexCount = 0;
    uint32_t indexBase = 0;
    uint32_t indexCount = 0;
  };
  std::vector<Mesh> meshes{};

  struct Dimension {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
  } dim;
};
