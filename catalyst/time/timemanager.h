#pragma once
#include <cstdint>
#include <map>
#include <vector>

namespace catalyst {
class TimeManager {
 public:
  class Clock {
   public:
    uint32_t clock_id_;
    float multiplier_;
    float elapsed_time_;
    float delta_time_;
  };
  Clock* base_clock_;
  Clock* game_clock_;
  static TimeManager& Get();
  void StartUp();
  void ShutDown();
  void Update();
  Clock& CreateClock(Clock& parent_clock, float multiplier = 1.0f);
  void DestroyClock(Clock& clock);

 private:
  class ClockNode {
   public:
    Clock clock_;
    ClockNode* parent;
    std::vector<ClockNode*> children_;
    ClockNode();
    void Increment(float delta_time);
  };
  ClockNode* clock_root_;
  uint32_t clock_counter_;
  std::map<uint32_t, ClockNode*> clock_nodes_;
  TimeManager();
};
}  // namespace catalyst