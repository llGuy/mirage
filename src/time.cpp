#include "time.hpp"
#include <GLFW/glfw3.h>

const time_data *gtime;

static time_data time_;

void init_time() {
    time_.frame_dt = 0.0f;
    time_.current_time = glfwGetTime();

    gtime = &time_;
}

void end_frame_time() {
    float prev_time = time_.current_time;
    time_.current_time = glfwGetTime();
    time_.frame_dt = time_.current_time - prev_time;
}
