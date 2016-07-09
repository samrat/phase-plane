typedef struct __vec2 {
  float x, y;
} vec2;

static vec2
vec2_add(vec2 a, vec2 b) {
  vec2 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;

  return result;
}

static vec2
vec2_scale(float t, vec2 a) {
  vec2 result;

  result.x = t * a.x;
  result.y = t * a.y;

  return result;
}
