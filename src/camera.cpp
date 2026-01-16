#include "camera.hpp"

#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/matrix.hpp>

using namespace vx;

void Camera::update(float dt) {
  if (action & CameraAction::MoveFront) {
    pos.x -= speed * sin(yaw) * dt;
    pos.z -= speed * cos(yaw) * dt;
  }
  if (action & CameraAction::MoveBack) {
    pos.x += speed * sin(yaw) * dt;
    pos.z += speed * cos(yaw) * dt;
  }
  if (action & CameraAction::MoveLeft) {
    pos.x -= speed * cos(yaw) * dt;
    pos.z += speed * sin(yaw) * dt;
  }
  if (action & CameraAction::MoveRight) {
    pos.x += speed * cos(yaw) * dt;
    pos.z -= speed * sin(yaw) * dt;
  }
  if (action & CameraAction::MoveUp) {
    pos.y += speed * dt;
  }
  if (action & CameraAction::MoveDown) {
    pos.y -= speed * dt;
  }

  action = CameraAction::None;
}

CameraUniforms Camera::uniforms(float width, float height) {
  // maths stolen from https://www.opengl-tutorial.org/beginners-tutorials/tutorial-6-keyboard-and-mouse/
  glm::vec3 direction(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
  float yaw_orth = yaw -  std::numbers::pi / 2;
  glm::vec3 right(sin(yaw_orth), 0, cos(yaw_orth));
  glm::vec3 up = glm::cross(right, direction);
  glm::mat4 view = glm::lookAt(pos, pos - direction, up);

  auto proj = glm::perspective(glm::radians(45.f), width / height, .1f, 10.f);
  proj[1][1] *= -1;

  return {
    .proj_view = proj * view,
    .view_inv = glm::inverse(view),
    .proj_view_inv = glm::inverse(proj * view),
    .viewport = {width, height},
    .tan_fov = static_cast<float>(tan(fov)),
    .z_near = z_near,
    .z_far = z_far,
    .max_marches = max_marches
  };
}
