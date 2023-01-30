#pragma once

extern const struct time_data {
    float frame_dt;
    float current_time;
} *gtime;

void init_time();
void end_frame_time();
