#pragma once

#include <GLFW/glfw3.h>
#include <stdexcept>

namespace vx {

class Window {
public:
  Window(int width, int height, const char *title);
  Window(const Window &window) = delete;
  Window &operator=(const Window &window) = delete;
  Window(const Window &&window) = delete;
  Window &operator=(const Window &&window) = delete;
  ~Window();

  static Window &get(GLFWwindow *window) {
    return *reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  }

  bool should_close() {
    return glfwWindowShouldClose(window_);
  }

  void poll_events() {
    glfwPollEvents();
  }

  void wait_events() {
    glfwWaitEvents();
  }

  void fb_size(int &width, int &height) {
    glfwGetFramebufferSize(window_, &width, &height);
  }

  bool has_framebuffer_resized() {
    bool old = fb_resized;
    fb_resized = false;
    return old;
  }

  void set_key_callback(GLFWkeyfun callback) {
    glfwSetKeyCallback(window_, callback);
  }

  void capture_cursor(bool capture) {
    if (capture) {
      glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
      else throw std::runtime_error("raw mouse motion unsupported");
      glfwGetCursorPos(window_, &xpos, &ypos);
    } else glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  bool is_cursor_captured() {
    return glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
  }

  void toggle_cursor_capture() {
    capture_cursor(!is_cursor_captured());
  }

  void delta_cursor(float &dx, float &dy) {
    double xpos_new, ypos_new;
    glfwGetCursorPos(window_, &xpos_new, &ypos_new);
    dx = xpos_new - xpos;
    dy = ypos_new - ypos;
    xpos = xpos_new;
    ypos = ypos_new;
  }

  float delta_time();

  int fps() { return fps_; }
  GLFWwindow *window() { return window_; }

private:
  GLFWwindow *window_;
  bool fb_resized;

  // time stats
  float last_time = 0.;
  float last_second = 0.;
  int fps_ = 0.;
  int frame_count = 0;

  // cursor
  double xpos, ypos;

  static void fb_resize_cb(GLFWwindow *window, int width, int height);
};

}
