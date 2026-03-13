#include "services/utilities/controller_status.h"

namespace {
    bool s_controller_bench_mode_enabled = false;
}

bool is_controller_bench_mode_enabled()
{
    return s_controller_bench_mode_enabled;
}

void set_controller_bench_mode_enabled(bool enabled)
{
    s_controller_bench_mode_enabled = enabled;
}
