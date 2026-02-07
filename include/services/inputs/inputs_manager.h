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

private:
  std::vector<Input*> inputs;
  std::vector<void(*)(uint32_t)> tasks;
};

extern InputManager inputs_manager;
