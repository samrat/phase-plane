#include <stdbool.h>
#include <math.h>

#define GLEW_STATIC
#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "shaders.c"

#define WIDTH 800
#define HEIGHT 600

static int num_rows = 20;
static int num_columns = 20;

typedef struct {
  float x, y, dirX, dirY;
} point_vertex;

static point_vertex *points;

typedef struct __vec2 {
  float x, y;
} vec2;

static struct {
  GLuint vertex_shader, fragment_shader, geometry_shader, shader_program;

  struct {
    GLint pos, dir;
  } attributes;

  struct {
    GLint scale;
  } uniforms;

  float minX, minY, maxX, maxY;
  float scaleX, scaleY;
} g_pplane_state;

vec2
foo(float x, float y) {
  vec2 result;
  result.x = x*x + y;
  result.y = x - y;

  return result;
}

vec2
unit_vector(vec2 v) {
  float m = sqrt(v.x*v.x + v.y*v.y);
  vec2 result;

  result.x = v.x / m;
  result.y = v.y / m;

  return result;
}


static void
show_info_log(GLuint object,
              PFNGLGETSHADERIVPROC glGet__iv,
              PFNGLGETSHADERINFOLOGPROC glGet__InfoLog) {
  GLint log_length;
  char *log;

  glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
  log = malloc(log_length);
  glGet__InfoLog(object, log_length, NULL, log);
  fprintf(stderr, "%s", log);
  free(log);
}


GLuint create_shader(GLenum type, const GLchar *src) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  GLint shader_ok;
  /* Check that shader compiled properly */
  glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
  if (!shader_ok) {
    fprintf(stderr, "Failed to compile %s:\n", src);
    show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

int
create_gl_resources() {
  g_pplane_state.vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_src);
  g_pplane_state.fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
  g_pplane_state.geometry_shader = create_shader(GL_GEOMETRY_SHADER, geometry_shader_src);

  g_pplane_state.shader_program = glCreateProgram();
  glAttachShader(g_pplane_state.shader_program, g_pplane_state.vertex_shader);
  glAttachShader(g_pplane_state.shader_program, g_pplane_state.fragment_shader);
  glAttachShader(g_pplane_state.shader_program, g_pplane_state.geometry_shader);
  glBindFragDataLocation(g_pplane_state.shader_program, 0, "outColor");
  glLinkProgram(g_pplane_state.shader_program);
  glUseProgram(g_pplane_state.shader_program);

  // Create VAO
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  /* Buffer data */
  GLuint vbo;
  glGenBuffers(1, &vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

  // Specify layout of point data
  g_pplane_state.attributes.pos = glGetAttribLocation(g_pplane_state.shader_program, "pos");
  glEnableVertexAttribArray(g_pplane_state.attributes.pos);
  glVertexAttribPointer(g_pplane_state.attributes.pos, 2, GL_FLOAT,
                        GL_FALSE, 4*sizeof(float), 0);

  g_pplane_state.attributes.dir = glGetAttribLocation(g_pplane_state.shader_program, "dir");
  glEnableVertexAttribArray(g_pplane_state.attributes.dir);
  glVertexAttribPointer(g_pplane_state.attributes.dir, 2, GL_FLOAT,
                        GL_FALSE, 4*sizeof(float),
                        (void*)(2*sizeof(float)));

  g_pplane_state.uniforms.scale = glGetUniformLocation(g_pplane_state.shader_program, "scale_factor");

  return 0;
}


vec2
canonical_mouse_pos() {
  vec2 result;
  int x, y;
  SDL_GetMouseState(&x, &y);

  result.x = 2*((float)x / WIDTH) - 1.0f;
  result.y = -(2*((float)y / HEIGHT) - 1.0f);

  return result;
}

vec2
real_to_canonical_coords(float x, float y) {
  vec2 result;
  result.x = x * g_pplane_state.scaleX;
  result.y = y * g_pplane_state.scaleY;

  return result;
}

vec2
canonical_to_real_coords(float x, float y) {
  vec2 result;
  result.x = x / g_pplane_state.scaleX;
  result.y = y / g_pplane_state.scaleY;

  return result;
}


int main() {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_Window* window = SDL_CreateWindow("pplane", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
  SDL_GLContext context = SDL_GL_CreateContext(window);

  glewExperimental = GL_TRUE;
  glewInit();

  /* NOTE: last point is for the cursor */
  int num_points = num_rows*num_columns+1;
  size_t points_size = sizeof(point_vertex) * num_points;
  points = malloc(points_size);

  /* Shaders and GLSL program */
  create_gl_resources();

  glUniform2f(g_pplane_state.uniforms.scale, 1.0, 1.0);

  vec2 start = {-0.5, -0.5};

  g_pplane_state.minX = -10.0;
  g_pplane_state.minY = -10.0;
  g_pplane_state.maxX = 10.0;
  g_pplane_state.maxY = 10.0;

  g_pplane_state.scaleX = 2 / (g_pplane_state.maxX - g_pplane_state.minX);
  g_pplane_state.scaleY = 2 / (g_pplane_state.maxY - g_pplane_state.minY);

  float stepX = (g_pplane_state.maxX - g_pplane_state.minX) / num_rows;
  float stepY = (g_pplane_state.maxY - g_pplane_state.minY) / num_columns;

  int index = 0;
  for (float i = g_pplane_state.minX;
       i < g_pplane_state.maxX;
       i += stepX) {
    for (float j = g_pplane_state.minY;
         j < g_pplane_state.maxY;
         j += stepY) {
      index += 1;
      vec2 arrow = unit_vector(foo(i, j));
      points[index].x = i * g_pplane_state.scaleX;
      points[index].y = j * g_pplane_state.scaleY;

      points[index].dirX = arrow.x * 0.05;
      points[index].dirY = arrow.y * 0.05;
    }
  }

  glBufferData(GL_ARRAY_BUFFER, points_size, points, GL_STATIC_DRAW);

  SDL_Event window_event;
  while (true) {
    if (SDL_PollEvent(&window_event)) {
      if (window_event.type == SDL_QUIT) break;
    }

    vec2 m = canonical_mouse_pos();
    vec2 arrow = unit_vector(foo(m.x/g_pplane_state.scaleX, m.y/g_pplane_state.scaleY));

    points[num_points-1].x = m.x;
    points[num_points-1].y = m.y;
    points[num_points-1].dirX = arrow.x;
    points[num_points-1].dirY = arrow.y;

    /* TODO: Can I update just the data at points[num_points-1] ? */
    glBufferSubData(GL_ARRAY_BUFFER, 0, points_size, points);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_POINTS, 0, num_points);

    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(context);
  SDL_Quit();
  return 0;
}
