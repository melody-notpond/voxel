#include "gameobj.hpp"

using namespace vx;

void GameObj::render(Renderer &render) {
  render.bind_shader_data(shader_data, uniforms);

  auto &commands = render.command_buffer();
  commands.bindVertexBuffers(0, *model.vertex_buffer(), {0});
  commands.bindIndexBuffer(model.index_buffer(), 0, vk::IndexType::eUint32);
  commands.drawIndexed(model.indices().size(), 1, 0, 0, 0);
}
