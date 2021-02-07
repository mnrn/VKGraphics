/**
 * #brief Assimpを使ったモデルローダ
 */

#include "VK/Model.h"

#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>

#include <algorithm>
#include <boost/assert.hpp>
#include <iostream>

#include "VK/Common.h"
#include "VK/Device.h"

static constexpr uint32_t defaultFlags =
    aiProcess_FlipWindingOrder | aiProcess_Triangulate |
    aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace |
    aiProcess_GenSmoothNormals;

bool Model::LoadFromFile(const Device &device, const std::string &filepath,
                         VkQueue copyQueue, const VertexLayout &vertexLayout,
                         const ModelCreateInfo &modelCreateInfo) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(filepath, defaultFlags);
  if (scene == nullptr) {
    std::cerr << importer.GetErrorString() << std::endl;
    BOOST_ASSERT_MSG(scene != nullptr, "Filed to load model!");
    return false;
  }

  const glm::vec3 center = modelCreateInfo.center;
  const glm::vec3 scale = modelCreateInfo.scale;
  const glm::vec2 uvscale = modelCreateInfo.uvscale;

  meshes.clear();
  meshes.resize(scene->mNumMeshes);

  std::vector<float> vertexBuffer;
  vertexCount = 0;
  std::vector<uint32_t> indexBuffer;
  indexCount = 0;
  for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
    const aiMesh *mesh = scene->mMeshes[i];

    meshes[i] = {};
    meshes[i].vertexBase = vertexCount;
    meshes[i].indexBase = indexCount;

    vertexCount += mesh->mNumVertices;

    aiColor3D color(0.0f, 0.0f, 0.0f);
    if (modelCreateInfo.color.has_value()) {
      color.r = modelCreateInfo.color->r;
      color.g = modelCreateInfo.color->g;
      color.b = modelCreateInfo.color->b;
    } else {
      scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE,
                                                   color);
    }
    const aiVector3D zero3D(0.0f, 0.0f, 0.0f);
    for (uint32_t j = 0; j < mesh->mNumVertices; j++) {

      const aiVector3D &pos = mesh->mVertices[j];
      const aiVector3D &norm = mesh->mNormals[j];
      const aiVector3D &uv =
          (mesh->HasTextureCoords(0)) ? mesh->mTextureCoords[0][j] : zero3D;
      const aiVector3D &tang =
          (mesh->HasTangentsAndBitangents()) ? mesh->mTangents[j] : zero3D;
      const aiVector3D &bitang =
          (mesh->HasTangentsAndBitangents()) ? mesh->mBitangents[j] : zero3D;

      for (const auto &component : vertexLayout.components) {
        switch (component) {
        case VertexLayoutComponent::Position:
          vertexBuffer.emplace_back(pos.x * scale.x + center.x);
          vertexBuffer.emplace_back(pos.y * scale.y + center.y);
          vertexBuffer.emplace_back(pos.z * scale.z + center.z);
          break;
        case VertexLayoutComponent::Normal:
          vertexBuffer.emplace_back(norm.x);
          vertexBuffer.emplace_back(norm.y);
          vertexBuffer.emplace_back(norm.z);
          break;
        case VertexLayoutComponent::UV:
          vertexBuffer.emplace_back(uv.x * uvscale.s);
          vertexBuffer.emplace_back(uv.y * uvscale.t);
          break;
        case VertexLayoutComponent::Color:
          vertexBuffer.emplace_back(color.r);
          vertexBuffer.emplace_back(color.g);
          vertexBuffer.emplace_back(color.b);
          break;
        case VertexLayoutComponent::Tangent:
          vertexBuffer.emplace_back(tang.x);
          vertexBuffer.emplace_back(tang.y);
          vertexBuffer.emplace_back(tang.z);
          break;
        case VertexLayoutComponent::Bitangent:
          vertexBuffer.emplace_back(bitang.x);
          vertexBuffer.emplace_back(bitang.y);
          vertexBuffer.emplace_back(bitang.z);
          break;
        case VertexLayoutComponent::DummyFloat:
          vertexBuffer.emplace_back(0.0f);
          break;
        case VertexLayoutComponent::DummyVec4:
          vertexBuffer.emplace_back(0.0f);
          vertexBuffer.emplace_back(0.0f);
          vertexBuffer.emplace_back(0.0f);
          vertexBuffer.emplace_back(0.0f);
          break;
        }
      }
      dim.min.x = std::min(pos.x, dim.min.x);
      dim.min.y = std::min(pos.y, dim.min.y);
      dim.min.z = std::min(pos.z, dim.min.z);

      dim.max.x = std::max(pos.x, dim.max.x);
      dim.max.y = std::max(pos.y, dim.max.y);
      dim.max.z = std::max(pos.z, dim.max.z);
    }
    meshes[i].vertexCount = mesh->mNumVertices;

    const auto indexBase = static_cast<uint32_t>(indexBuffer.size());
    for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
      const aiFace &face = mesh->mFaces[j];
      if (face.mNumIndices != 3) {
        continue;
      }
      indexBuffer.emplace_back(indexBase + face.mIndices[0]);
      indexBuffer.emplace_back(indexBase + face.mIndices[1]);
      indexBuffer.emplace_back(indexBase + face.mIndices[2]);
      meshes[i].indexCount += 3;
      indexCount += 3;
    }
  }

  const auto vtxBufSize =
      static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
  const auto idxBufSize =
      static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

  // ステージングバッファを使用して、頂点バッファとインデックスバッファをデバイスのローカルメモリに移動します。
  Buffer vertexStaging;
  VK_CHECK_RESULT(vertexStaging.Create(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       vertexBuffer.data(), vtxBufSize));
  Buffer indexStaging;
  VK_CHECK_RESULT(indexStaging.Create(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      indexBuffer.data(), idxBufSize));

  // デバイスのローカルターゲットバッファを生成します。
  VK_CHECK_RESULT(vertices.Create(
      device,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | modelCreateInfo.memoryPropertyFlags,
      vtxBufSize));
  VK_CHECK_RESULT(indices.Create(
      device,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | modelCreateInfo.memoryPropertyFlags,
      idxBufSize));

  // ステージングバッファからコピーします。
  VkCommandBuffer copyCmd = device.CreateCommandBuffer();
  VkBufferCopy copyRegion{};

  copyRegion.size = vtxBufSize;
  vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1,
                  &copyRegion);

  copyRegion.size = idxBufSize;
  vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

  device.FlushCommandBuffer(copyCmd, copyQueue);

  // ステージングリソースを解放します。
  indexStaging.Destroy(device);
  vertexStaging.Destroy(device);

  return true;
}

void Model::Destroy(const Device &device) const {
  indices.Destroy(device);
  vertices.Destroy(device);
}
