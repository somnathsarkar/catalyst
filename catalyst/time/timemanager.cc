#include <catalyst/time/timemanager.h>

#include <chrono>

#include <catalyst/dev/dev.h>

namespace catalyst {
TimeManager& TimeManager::Get() {
  static TimeManager singleton;
  return singleton;
}
TimeManager::TimeManager(){};
void TimeManager::StartUp() {
  clock_root_ = new ClockNode();
  (clock_root_->clock_).clock_id_ = 0;
  (clock_root_->clock_).multiplier_ = 1.0f;
  (clock_root_->clock_).elapsed_time_ = 0.0f;
  (clock_root_->clock_).delta_time_ = 0.0f;
  clock_root_->parent = nullptr;
  clock_nodes_[clock_counter_] = clock_root_;
  clock_counter_ = 1;
  base_clock_ = &(clock_root_->clock_);
  CreateClock(*base_clock_, 1.0f);
  game_clock_ = &(clock_nodes_[1]->clock_);
  Update();
}
void TimeManager::ShutDown() {
  DestroyClock(*game_clock_);
  clock_nodes_.erase(0);
  ASSERT((clock_nodes_).empty(), "All clocks were not destroyed!");
  delete clock_root_;
}
void TimeManager::Update() {
  static auto start_time = std::chrono::high_resolution_clock::now();
  auto current_time = std::chrono::high_resolution_clock::now();
  float base_elapsed_time =
      std::chrono::duration<float, std::chrono::seconds::period>(current_time -
                                                                 start_time)
          .count();
  float base_delta_time =
      base_elapsed_time - ((clock_root_->clock_).elapsed_time_);
  clock_root_->Increment(base_delta_time);
}
TimeManager::Clock& TimeManager::CreateClock(TimeManager::Clock& parent_clock,
                                             float multiplier) {
  ClockNode* parent_node = clock_nodes_[parent_clock.clock_id_];
  ClockNode* new_node = new ClockNode();
  (new_node->clock_).clock_id_ = clock_counter_;
  (new_node->clock_).multiplier_ = multiplier;
  (new_node->clock_).elapsed_time_ = parent_clock.elapsed_time_;
  (new_node->clock_).delta_time_ = parent_clock.delta_time_;
  new_node->parent = parent_node;
  parent_node->children_.push_back(new_node);
  clock_nodes_[clock_counter_] = new_node;
  return new_node->clock_;
}
void TimeManager::DestroyClock(Clock& clock) {
  ClockNode* node = clock_nodes_[clock.clock_id_];
  ASSERT(clock.clock_id_ != 0, "Attempting to destroy Base Clock!");
  ASSERT((node->children_).empty(),
         "Attempting to destroy Clock with existing children!");
  clock_nodes_.erase(clock.clock_id_);
  delete node;
}
void TimeManager::ClockNode::Increment(float delta_time) {
  delta_time *= clock_.multiplier_;
  clock_.elapsed_time_ += delta_time;
  clock_.delta_time_ = delta_time;
  for (auto& child_clock_node : children_) {
    child_clock_node->Increment(delta_time);
  }
}
TimeManager::ClockNode::ClockNode() {}
}  // namespace catalyst