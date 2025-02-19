#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>
#include <memory>
#include <csignal>

#include "audio.hpp"
#include "midi.hpp"

class Application;
static Application* g_app = nullptr;
static void signal_handler(int);

class Application {
 public:
  Application() {
    g_app = this;
    signal(SIGINT, signal_handler);

    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    window_ = glfwCreateWindow(1280, 1440, "FM Synth", nullptr, nullptr);
    if (!window_) {
      glfwTerminate();
      throw std::runtime_error("Failed to create window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    audio_ = std::make_unique<Audio>();
    midi_ = std::make_unique<Midi>(*audio_);
    if (!midi_->open()) {
      throw std::runtime_error("Failed to open MIDI device");
    }
  }

  void quit() {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }

  ~Application() {
    audio_->saveParameters("params.txt");
    audio_.reset();
    midi_.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window_);
    glfwTerminate();
  }

  void run() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      renderGui();

      ImGui::Render();
      int display_w, display_h;
      glfwGetFramebufferSize(window_, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window_);
    }
  }

 private:
  void renderGui() {
    ImGui::Begin("FM Synth Parameters");
    audio_->gui();
    ImGui::End();
  }

  GLFWwindow* window_;
  std::unique_ptr<Audio> audio_;
  std::unique_ptr<Midi> midi_;
};

static void signal_handler(int) {
  if (g_app) g_app->quit();
}

int main() {
  try {
    Application app;
    app.run();
  } catch (const std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }
  return 0;
} 