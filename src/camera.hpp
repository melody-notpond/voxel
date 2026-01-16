#pragma once

#include <glm/glm.hpp>

namespace vx {

struct CameraUniforms {
  glm::mat4 proj_view;
  glm::mat4 view_inv;
  glm::mat4 proj_view_inv;
  glm::vec2 viewport;
  float tan_fov;
  float z_near;
  float z_far;
  int max_marches;
};

enum CameraAction : int {
  None        = 0b000000,
  MoveFront   = 0b000001,
  MoveBack    = 0b000010,
  MoveLeft    = 0b000100,
  MoveRight   = 0b001000,
  MoveUp      = 0b010000,
  MoveDown    = 0b100000,
};

inline constexpr CameraAction operator~(CameraAction a) {
  return static_cast<CameraAction>(~static_cast<int>(a));
}

inline CameraAction &operator&=(CameraAction &a, CameraAction b) {
  a = static_cast<CameraAction>(a & b);
  return a;
}

inline CameraAction &operator|=(CameraAction &a, CameraAction b) {
  a = static_cast<CameraAction>(a | b);
  return a;
}

class Camera {
public:
  void set_action(CameraAction action) { this->action = action; }

  void rotate(float dx, float dy) {
    yaw -= dx * sensitivity.x;
    if (yaw >= DEGREES_360)
      yaw -= DEGREES_360;
    else if (yaw <= -DEGREES_360)
      yaw += DEGREES_360;

    pitch += dy * sensitivity.y;
    if (pitch >= DEGREES_90)
      pitch = DEGREES_90;
    else if (pitch <= -DEGREES_90)
      pitch = -DEGREES_90;
  }

  void update(float dt);

  CameraUniforms uniforms(float width, float height);
private:
  static constexpr float DEGREES_90  = glm::radians(90.);
  static constexpr float DEGREES_360 = glm::radians(360.);

  glm::vec3 pos = {0, 0, 2};
  float pitch; // pitch is up-down
  float yaw; // yaw is left-right

  CameraAction action;
  float speed = 2.;
  glm::vec2 sensitivity = {0.005, 0.005};

  float fov = glm::radians(45.);
  float z_near = .1;
  float z_far = 100.;
  int max_marches = 80;
};

}
