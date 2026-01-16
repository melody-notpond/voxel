#include "camera.hpp"
#include "chunk.hpp"
#include "device.hpp"
#include "gameobj.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "window.hpp"
#include <vulkan/vulkan_raii.hpp>

using namespace std;

void update_uniforms(vx::Renderer &render, glm::mat4 view, glm::mat4 proj,
  vx::GameObj &obj) {
  // vx::UniformData ubo {
  //   .model = glm::translate(glm::mat4(1.), { 0., 0., -5. }),
  //   .view = view,
  //   .proj = proj
  // };
  // obj.uniforms = ubo;
}

vx::CameraAction cam_action = vx::CameraAction::None;

void key_callback(
  GLFWwindow *window,
  int key,
  int scancode,
  int action,
  int mods
) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    vx::Window::get(window).toggle_cursor_capture();
    return;
  }

  if (!vx::Window::get(window).is_cursor_captured())
    return;

  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_W:
        cam_action |= vx::CameraAction::MoveFront;
        break;
      case GLFW_KEY_A:
        cam_action |= vx::CameraAction::MoveLeft;
        break;
      case GLFW_KEY_S:
        cam_action |= vx::CameraAction::MoveBack;
        break;
      case GLFW_KEY_D:
        cam_action |= vx::CameraAction::MoveRight;
        break;
      case GLFW_KEY_SPACE:
        cam_action |= vx::CameraAction::MoveUp;
        break;
      case GLFW_KEY_LEFT_SHIFT:
        cam_action |= vx::CameraAction::MoveDown;
        break;
    }
  } else if (action == GLFW_RELEASE) {
    switch (key) {
      case GLFW_KEY_W:
        cam_action &= ~vx::CameraAction::MoveFront;
        break;
      case GLFW_KEY_A:
        cam_action &= ~vx::CameraAction::MoveLeft;
        break;
      case GLFW_KEY_S:
        cam_action &= ~vx::CameraAction::MoveBack;
        break;
      case GLFW_KEY_D:
        cam_action &= ~vx::CameraAction::MoveRight;
        break;
      case GLFW_KEY_SPACE:
        cam_action &= ~vx::CameraAction::MoveUp;
        break;
      case GLFW_KEY_LEFT_SHIFT:
        cam_action &= ~vx::CameraAction::MoveDown;
        break;
    }
  }
}

// TODO
// - start working on voxel
int main(void) {
  vx::Window window(800, 600, "voxels");
  window.set_key_callback(key_callback);
  vx::Device device(window);
  vx::Renderer render(window, device, 2);
  // vx::Texture texture(render, "assets/viking_room.png");
  // vx::Model model(render, "assets/viking_room.obj");
  vx::Chunk chunk(render, 0, 0, 0);

  vx::Camera camera;

  // vx::GameObj obj {
  //   .model = model,
  //   .shader_data = render.create_shader_data(texture),
  // };

  while (!window.should_close()) {
    float dt = window.delta_time();

    // update
    if (window.is_cursor_captured()) {
      float dx, dy;
      window.delta_cursor(dx, dy);
      camera.rotate(dx, dy);
      camera.set_action(cam_action);
      camera.update(dt);
    }

    // render
    if (!render.begin_frame(camera))
      continue;
    chunk.render(render);
    // glm::mat4 view = camera.view_mat();
    // glm::mat4 proj = camera.proj_mat(render.aspect_ratio());
    // update_uniforms(render, view, proj, obj);
    // obj.render(render);
    render.end_frame();
    window.poll_events();
  }

  device.wait();

  return 0;
}
