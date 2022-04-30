#pragma once
#include <catalyst/application/application.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <catalyst/time/timemanager.h>
#include <catalyst/render/renderer.h>
#include <catalyst/script/script.h>

namespace catalyst {
void Application::StartUp(int argc, char** argv) {
  ASSERT(main_window != nullptr, "Haven't assigned window!");
  this->argc = argc;
  this->argv = argv;
  glfwInit();
  TimeManager& time_manager = TimeManager::Get();
  time_manager.StartUp();
  main_window->StartUp(argc,argv);
  renderer = new Renderer(this);
  renderer->StartUp();
};
void Application::AssignWindow(Window* window) {
  ASSERT(main_window == nullptr, "Already assigned a window!");
  main_window = window;
}
void Application::LoadScene(Scene* scene) {
  scene_ = scene;
  main_window->LoadScene(scene_);
  renderer->LoadScene(*scene_);
}
void Application::LoadScript(Script* script) {
  script_ = script;
  script_->StartUp(*main_window,*scene_);
}
void Application::ShutDown() {
  renderer->EarlyShutDown();
  main_window->ShutDown();
  renderer->LateShutDown();
  TimeManager& time_manager = TimeManager::Get();
  time_manager.ShutDown();
  glfwTerminate();
}
void Application::Update() {
  main_window->Update();
  if (script_!=nullptr)
    script_->Update(*main_window,*scene_);
  TimeManager& time_manager = TimeManager::Get();
  time_manager.Update();
  renderer->Update();
}
Application::Application() : main_window(nullptr), scene_(nullptr),script_(nullptr) {}
}  // namespace catalyst