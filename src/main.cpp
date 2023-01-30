#include "time.hpp"
#include "core_render.hpp"
#include "render_context.hpp"

int main(int argc, char **argv) {
    init_render_context();
    init_core_render();
    init_time();

    while (is_running()) {
        run_render();
        end_frame_time();
    }

    return 0;
}
