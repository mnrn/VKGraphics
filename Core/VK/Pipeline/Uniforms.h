#pragma once

#include <glm/glm.hpp>
#include <vector>

struct PushConstants {
  virtual ~PushConstants() = defualt;
  virtual uint32_t GetVertexOffset() const { return 0; }
  virtual uint32_t GetVertexSize() const { return 0; }
  virtual uint32_t GetFragmentOffset() const { return 0; }
  virtual uint32_t GetFragmentSize() const { return 0; }
  virtual uint32_t GetFragmentSize() const { return 0; }
  virtual uint32_t GetTotalSize() const {
    return GetVertexSize() + GetFragmentSize();
  }
};
