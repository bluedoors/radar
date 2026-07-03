#pragma once
struct Point { int x; int y; };
Point project(float dst_km, float dir_deg, float range_km);
int ring_index(float dst_km, float range_km);
bool tag_visible(int ring_idx);
