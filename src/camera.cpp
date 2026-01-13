#include "camera.hpp"

#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <numbers>

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

glm::mat4 Camera::view_mat() {
  // maths stolen from https://www.opengl-tutorial.org/beginners-tutorials/tutorial-6-keyboard-and-mouse/
  glm::vec3 direction(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
  float yaw_orth = yaw -  std::numbers::pi / 2;
  glm::vec3 right(sin(yaw_orth), 0, cos(yaw_orth));
  glm::vec3 up = glm::cross(right, direction);
  return glm::lookAt(pos, pos - direction, up);
}

glm::mat4 Camera::proj_mat(float aspect_ratio) {
  // note: left handed coords with +y down +z out
  auto proj = glm::perspective(fov, aspect_ratio, z_near, z_far);
  proj[1][1] *= -1;
  return proj;
}
