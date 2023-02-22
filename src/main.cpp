#include "bump_alloc.h"
#include "debug_overlay.hpp"
#include "io.hpp"
#include "memory.hpp"
#include "time.hpp"
#include "core_render.hpp"
#include "render_context.hpp"

int main(int argc, char **argv) {
    init_bump_allocator(megabytes(10));
    init_render_context();
    init_core_render();
    init_io_raw();
    init_time();

    while (is_running()) {
        tick_io_raw();

        if (!overlay_has_focus())
            tick_debug_viewer();
        run_render();
        bump_clear();
        end_frame_time();
    }

    return 0;
}
