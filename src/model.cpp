#include "model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace vx;

void Model::load_model(const char *path) {
  tinyobj::attrib_t attrs;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrs, &shapes, &materials, &warn, &err, path))
    throw std::runtime_error(warn + "\n" + err);

  std::unordered_map<Vertex, uint32_t> uniqueVertices;
  for (const auto &shape : shapes) {
    for (const auto index : shape.mesh.indices) {
      Vertex vertex {
        .pos = {
          attrs.vertices[3 * index.vertex_index + 0],
          attrs.vertices[3 * index.vertex_index + 1],
          attrs.vertices[3 * index.vertex_index + 2]
        },
        .color = {1., 1., 1.},
        .texCoord = {
          attrs.texcoords[2 * index.texcoord_index + 0],
          1. - attrs.texcoords[2 * index.texcoord_index + 1]
        }
      };

      if (!uniqueVertices.contains(vertex)) {
        uint32_t index = vertices_.size();
        vertices_.push_back(vertex);
        indices_.push_back(index);
        uniqueVertices[vertex] = index;
      } else indices_.push_back(uniqueVertices[vertex]);
    }
  }
}

void Model::create_vertex_buffer(vx::CommandPool &pool) {
  vk::DeviceSize size = sizeof(vertices_[0]) * vertices_.size();
  pool.copy_to_buffer_staged(vbuffer, vmem,
    vk::BufferUsageFlagBits::eVertexBuffer, vertices_.data(), size);
}

void Model::create_index_buffer(vx::CommandPool &pool) {
  vk::DeviceSize size = sizeof(indices_[0]) * indices_.size();
  pool.copy_to_buffer_staged(ibuffer, imem,
    vk::BufferUsageFlagBits::eIndexBuffer, indices_.data(), size);
}
