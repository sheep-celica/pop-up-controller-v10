// input_manager.cpp
#include <vector>
#include <Arduino.h>
#include "services/inputs/types/input.h"
#include "services/inputs/inputs_manager.h"


// The one and only instance of InputManager
InputManager inputs_manager;


// InputManager definition
void InputManager::add(Input& input)
{
    input.begin();
    inputs.push_back(&input);
}

void InputManager::add_task(void (*task)(uint32_t now_ms))
{
    tasks.push_back(task);
}

void InputManager::reserve(size_t n_inputs, size_t n_tasks)
{
    inputs.reserve(n_inputs);
    tasks.reserve(n_tasks);
}

size_t InputManager::input_count() const
{
    return inputs.size();
}

size_t InputManager::task_count() const
{
    return tasks.size();
}

void InputManager::update()
{
    const uint32_t now = millis();

    for (auto* in : inputs)
    {
        in->update(now);
    }

    for (auto* task : tasks)
    {
        task(now);
    }
}
