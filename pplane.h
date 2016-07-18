/* TODO: These will become configurable from the UI */
#define HALF_NUM_STEPS_PER_SOLUTION 800
#define SOLUTION_DT 0.01f

#define MAX_SOLUTIONS 20


typedef struct {
  float x, y, dirX, dirY;
} point_vertex;

typedef struct {
struct {
    float endpoints[6][2];

    GLuint vertex_shader, fragment_shader, shader_program;
    GLuint vao, vbo;

    struct {
      GLint pos;
    } attributes;

    struct {
      GLint scale;
    } uniforms;
  } axes;

  struct {
    int num_solutions;
    bool recompute_solutions;

    /* TODO storage for solutions should get resized when necessary */
    float solutions[MAX_SOLUTIONS][HALF_NUM_STEPS_PER_SOLUTION*2][2];
    float init[MAX_SOLUTIONS][2];

    GLuint vertex_shader, fragment_shader, shader_program;
    GLuint vao, vbo;

    struct {
      GLint pos;
    } attributes;

    struct {
      GLint scale;
    } uniforms;
  } solutions;

  struct {
    GLuint vertex_shader, fragment_shader, geometry_shader, shader_program;

    GLuint vao, vbo;

    struct {
      GLint pos, dir;
    } attributes;

    struct {
      GLint scale;
    } uniforms;

    point_vertex *points;
  } plane;
} gl_state_t;

typedef struct {
  gl_state_t *gl_state;

  int32_t num_points;
  size_t points_size;

  float minX, minY, maxX, maxY;
  float scaleX, scaleY;
  float translateX, translateY;
} pplane_state_t;
