#include "vec2.c"
#include "pplane.h"

vec2 diffeq_system(pplane_state_t *pplane_state, vec2 current);

vec2
rk4_weighted_avg(vec2 a, vec2 b, vec2 c, vec2 d) {
  vec2 result;

  result.x = (a.x + 2*b.x + 2*c.x + d.x) / 6.0;
  result.y = (a.y + 2*b.y + 2*c.y + d.y) / 6.0;

  return result;
}

/* Compute next step of autonomous differential equation. */
vec2
rk4(pplane_state_t *pplane_state, vec2 current, float dt) {
  vec2 k1 = diffeq_system(pplane_state, current);
  vec2 k2 = diffeq_system(pplane_state,
                          vec2_add(current, vec2_scale(dt/2, k1)));
  vec2 k3 = diffeq_system(pplane_state,
                          vec2_add(current, vec2_scale(dt/2, k2)));
  vec2 k4 = diffeq_system(pplane_state,
                          vec2_add(current, vec2_scale(dt, k3)));

  vec2 k = rk4_weighted_avg(k1, k2, k3, k4);

  vec2 result = vec2_add(current, vec2_scale(dt, k));

  return result;
}
