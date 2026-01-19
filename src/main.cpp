#include "camera.hpp"
#include "chunk.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "window.hpp"
#include <vulkan/vulkan_raii.hpp>

using namespace std;

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

// TODO:
// - sparse voxel octrees
int main(void) {
  vx::Window window(800, 600, "voxels");
  window.set_key_callback(key_callback);
  vx::Device device(window);
  vx::Renderer render(window, device, 3);
  vx::Chunk chunk(render, 0, 0, -2);
  vx::Chunk chunk2(render, 0, 0, 0);
  vx::Camera camera;

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
    chunk2.render(render);
    render.end_frame();
    window.poll_events();
  }

  device.wait();

  return 0;
}
