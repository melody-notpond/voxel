#pragma once

#include "renderer.hpp"
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "device.hpp"

namespace vx {

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static vk::VertexInputBindingDescription getBindingDesc() {
    return {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = vk::VertexInputRate::eVertex,
    };
  }

  static std::array<vk::VertexInputAttributeDescription, 3> getAttrsDescs() {
    return {
      vk::VertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, pos)
      },
      vk::VertexInputAttributeDescription {
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, color)
      },
      vk::VertexInputAttributeDescription {
        .location = 2,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(Vertex, texCoord)
      }
    };
  }

  bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color &&
      texCoord == other.texCoord;
  }
};

class Model {
public:
  Model(vx::Renderer &render, const char *path) {
    load_model(path);
    create_vertex_buffer(render.pool());
    create_index_buffer(render.pool());
  }

  std::vector<Vertex> &vertices() { return vertices_; }
  std::vector<uint32_t> &indices() { return indices_; }
  vk::raii::Buffer &vertex_buffer() { return vbuffer; }
  vk::raii::DeviceMemory &vertex_mem() { return vmem; }
  vk::raii::Buffer &index_buffer() { return ibuffer; }
  vk::raii::DeviceMemory &index_mem() { return imem; }

private:
  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;

  vk::raii::Buffer vbuffer = nullptr;
  vk::raii::DeviceMemory vmem = nullptr;
  vk::raii::Buffer ibuffer = nullptr;
  vk::raii::DeviceMemory imem = nullptr;

  void load_model(const char *path);
  void create_vertex_buffer(vx::CommandPool &pool);
  void create_index_buffer(vx::CommandPool &pool);
};

}

namespace std {

template<> struct hash<vx::Vertex> {
  size_t operator()(vx::Vertex const& vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
      (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
      (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};

}
