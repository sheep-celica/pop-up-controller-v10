// input_manager.h
#pragma once

#include <vector>
#include <Arduino.h>
#include "services/inputs/types/input.h"

class InputManager {
public:
  void add(Input& input);
  void add_task(void (*task)(uint32_t now_ms));
  void update();
  void reserve(size_t n_inputs, size_t n_tasks);

  size_t input_count() const;
  size_t task_count() const;
  
private:
  std::vector<Input*> inputs;
  std::vector<void(*)(uint32_t)> tasks;
};

extern InputManager inputs_manager;
