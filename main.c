#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include <GL/gl3w.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

#include "shaders.c"
#include "solver.c"

#define WIDTH 800
#define HEIGHT 600

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

static const int num_rows = 20;
static const int num_columns = 20;

typedef struct {
  float x, y, dirX, dirY;
} point_vertex;

static point_vertex *points;

static struct {
  GLuint vertex_shader, fragment_shader, geometry_shader, shader_program;

  GLuint vao, vbo;

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
    float solutions[10][800][2];

    GLuint vertex_shader, fragment_shader, shader_program;
    GLuint vao, vbo;

    struct {
      GLint pos;
    } attributes;

    struct {
      GLint scale;
    } uniforms;
  } solutions;


  int32_t num_points;
  size_t points_size;

  struct {
    GLint pos, dir;
  } attributes;

  struct {
    GLint scale;
  } uniforms;

  float minX, minY, maxX, maxY;
  float scaleX, scaleY;
  float translateX, translateY;
} g_pplane_state;

vec2
diffeq_system(vec2 current) {
  float x = current.x;
  float y = current.y;
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


GLuint
create_shader(GLenum type, const GLchar *src) {
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
create_axes_gl_state() {
  g_pplane_state.axes.vertex_shader = create_shader(GL_VERTEX_SHADER, axes_vertex_shader_src);
  g_pplane_state.axes.fragment_shader = create_shader(GL_FRAGMENT_SHADER, axes_fragment_shader_src);
  g_pplane_state.axes.shader_program = glCreateProgram();
  glAttachShader(g_pplane_state.axes.shader_program, g_pplane_state.axes.vertex_shader);
  glAttachShader(g_pplane_state.axes.shader_program, g_pplane_state.axes.fragment_shader);
  glBindFragDataLocation(g_pplane_state.axes.shader_program, 0, "outColor");
  glLinkProgram(g_pplane_state.axes.shader_program);
  glUseProgram(g_pplane_state.axes.shader_program);

  glGenVertexArrays(1, &g_pplane_state.axes.vao);
  glBindVertexArray(g_pplane_state.axes.vao);

  glGenBuffers(1, &g_pplane_state.axes.vbo);

  glBindBuffer(GL_ARRAY_BUFFER, g_pplane_state.axes.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_pplane_state.axes.endpoints),
               g_pplane_state.axes.endpoints, GL_STATIC_DRAW);

  g_pplane_state.axes.attributes.pos =
    glGetAttribLocation(g_pplane_state.axes.shader_program, "pos");
  glEnableVertexAttribArray(g_pplane_state.axes.attributes.pos);
  glVertexAttribPointer(g_pplane_state.axes.attributes.pos, 2, GL_FLOAT,
                        GL_FALSE, 0, 0);
  g_pplane_state.axes.uniforms.scale =
    glGetUniformLocation(g_pplane_state.axes.shader_program, "scale_factor");

  return 0;
}

int
create_solutions_gl_state() {
  g_pplane_state.solutions.vertex_shader = create_shader(GL_VERTEX_SHADER, axes_vertex_shader_src);
  g_pplane_state.solutions.fragment_shader = create_shader(GL_FRAGMENT_SHADER, axes_fragment_shader_src);
  g_pplane_state.solutions.shader_program = glCreateProgram();
  glAttachShader(g_pplane_state.solutions.shader_program, g_pplane_state.solutions.vertex_shader);
  glAttachShader(g_pplane_state.solutions.shader_program, g_pplane_state.solutions.fragment_shader);
  glBindFragDataLocation(g_pplane_state.solutions.shader_program, 0, "outColor");
  glLinkProgram(g_pplane_state.solutions.shader_program);
  glUseProgram(g_pplane_state.solutions.shader_program);

  glGenVertexArrays(1, &g_pplane_state.solutions.vao);
  glBindVertexArray(g_pplane_state.solutions.vao);

  glGenBuffers(1, &g_pplane_state.solutions.vbo);

  glBindBuffer(GL_ARRAY_BUFFER, g_pplane_state.solutions.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_pplane_state.solutions.solutions),
               g_pplane_state.solutions.solutions, GL_STATIC_DRAW);

  g_pplane_state.solutions.attributes.pos =
    glGetAttribLocation(g_pplane_state.solutions.shader_program, "pos");
  glEnableVertexAttribArray(g_pplane_state.solutions.attributes.pos);
  glVertexAttribPointer(g_pplane_state.solutions.attributes.pos, 2, GL_FLOAT,
                        GL_FALSE, 0, 0);
  g_pplane_state.solutions.uniforms.scale =
    glGetUniformLocation(g_pplane_state.solutions.shader_program, "scale_factor");

  return 0;
}

int
create_plane_gl_resources() {
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
  glGenVertexArrays(1, &g_pplane_state.vao);
  glBindVertexArray(g_pplane_state.vao);

  /* Buffer data */
  glGenBuffers(1, &g_pplane_state.vbo);

  glBindBuffer(GL_ARRAY_BUFFER, g_pplane_state.vbo);
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

int
create_gl_resources() {
  /* Plane */
  create_plane_gl_resources();
  /* Axes */
  create_axes_gl_state();
  /* Solutions */
  create_solutions_gl_state();

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
  result.x = (x * g_pplane_state.scaleX) - g_pplane_state.translateX;
  result.y = (y * g_pplane_state.scaleY) - g_pplane_state.translateY;

  return result;
}

vec2
canonical_to_real_coords(float x, float y) {
  vec2 result;
  result.x = (x + g_pplane_state.translateX) / g_pplane_state.scaleX;
  result.y = (y + g_pplane_state.translateY) / g_pplane_state.scaleY;

  return result;
}

static void
render() {
  glUseProgram(g_pplane_state.shader_program);
  glBindVertexArray(g_pplane_state.vao);
  glBindBuffer(GL_ARRAY_BUFFER, g_pplane_state.vbo);

  /* TODO: Can I update just the data at points[num_points-1] ? */
  glBufferSubData(GL_ARRAY_BUFFER, 0, g_pplane_state.points_size, points);
  glDrawArrays(GL_POINTS, 0, g_pplane_state.num_points);

  /* Axes */
  glUseProgram(g_pplane_state.axes.shader_program);
  glBindVertexArray(g_pplane_state.axes.vao);
  glBindBuffer(GL_ARRAY_BUFFER, g_pplane_state.axes.vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(g_pplane_state.axes.endpoints), g_pplane_state.axes.endpoints);
  glUniform2f(g_pplane_state.axes.uniforms.scale, 1.0, 1.0);

  glDrawArrays(GL_LINE_STRIP, 0, 6);

  /* Solutions */
  glUseProgram(g_pplane_state.solutions.shader_program);
  glBindVertexArray(g_pplane_state.solutions.vao);
  glBindBuffer(GL_ARRAY_BUFFER, g_pplane_state.solutions.vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(g_pplane_state.solutions.solutions), g_pplane_state.solutions.solutions);
  glUniform2f(g_pplane_state.solutions.uniforms.scale, 1.0, 1.0);

  glDrawArrays(GL_LINE_STRIP, 0, 800);


  nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
}

static void
recompute_scale_and_translate() {
  g_pplane_state.scaleX = 2.0f / (g_pplane_state.maxX - g_pplane_state.minX);
  g_pplane_state.scaleY = 2.0f / (g_pplane_state.maxY - g_pplane_state.minY);

  g_pplane_state.translateX = 1.0f - (-2*g_pplane_state.minX /
                                      (g_pplane_state.maxX - g_pplane_state.minX));
  g_pplane_state.translateY = 1.0f - (-2*g_pplane_state.minY /
                                      (g_pplane_state.maxY - g_pplane_state.minY));

}

static void
fill_plane_data() {
  vec2 min = canonical_to_real_coords(-1.0, -1.0);
  vec2 max = canonical_to_real_coords(1.0, 1.0);

  float stepX = (max.x - min.x) / num_rows;
  float stepY = (max.y - min.y) / num_columns;

  int index = 0;
  for (float i = min.x;
       i < max.x;
       i += stepX) {
    for (float j = min.y;
         j < max.y;
         j += stepY) {
      vec2 v = {.x = i, .y = j };
      vec2 arrow = unit_vector(diffeq_system(v));
      vec2 canon_coords = real_to_canonical_coords(i, j);
      points[index].x = canon_coords.x;
      points[index].y = canon_coords.y;

      points[index].dirX = arrow.x * 0.05;
      points[index].dirY = arrow.y * 0.05;

      index += 1;
    }
  }
}

static void
fill_axes_data() {
  vec2 left = real_to_canonical_coords(g_pplane_state.minX, 0);
  g_pplane_state.axes.endpoints[0][0] = left.x;
  g_pplane_state.axes.endpoints[0][1] = left.y;

  vec2 center = real_to_canonical_coords(0, 0);
  g_pplane_state.axes.endpoints[1][0] = center.x;
  g_pplane_state.axes.endpoints[1][1] = center.y;

  vec2 top = real_to_canonical_coords(0, g_pplane_state.maxY);
  g_pplane_state.axes.endpoints[2][0] = top.x;
  g_pplane_state.axes.endpoints[2][1] = top.y;

  vec2 bottom = real_to_canonical_coords(0, g_pplane_state.minY);
  g_pplane_state.axes.endpoints[3][0] = bottom.x;
  g_pplane_state.axes.endpoints[3][1] = bottom.y;

  g_pplane_state.axes.endpoints[4][0] = center.x;
  g_pplane_state.axes.endpoints[4][1] = center.y;

  vec2 right = real_to_canonical_coords(g_pplane_state.maxX, 0);
  g_pplane_state.axes.endpoints[5][0] = right.x;
  g_pplane_state.axes.endpoints[5][1] = right.y;
}

static void
set_mouse_position() {
  vec2 m = canonical_mouse_pos();
  vec2 real_m = canonical_to_real_coords(m.x, m.y);
  vec2 arrow = unit_vector(diffeq_system(real_m));

  points[g_pplane_state.num_points-1].x = m.x;
  points[g_pplane_state.num_points-1].y = m.y;
  points[g_pplane_state.num_points-1].dirX = arrow.x;
  points[g_pplane_state.num_points-1].dirY = arrow.y;
}


int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_Window* window = SDL_CreateWindow("pplane", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
  SDL_GLContext context = SDL_GL_CreateContext(window);
  int win_width, win_height;
  SDL_GetWindowSize(window, &win_width, &win_height);

  if (gl3wInit() != 0) {
    fprintf(stderr, "GL3W: failed to initialize\n");
    return 1;
  }


  /* GUI */
  struct nk_context *ctx;
  glViewport(0, 0, WIDTH, HEIGHT);
  ctx = nk_sdl_init(window);

  /* Load Fonts: if none of these are loaded a default font will be used  */
  {struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();}

  struct nk_color background = nk_rgb(28,48,62);

  /* NOTE: last point is for the cursor */
  g_pplane_state.num_points = num_rows*num_columns + 1;
  g_pplane_state.points_size = sizeof(point_vertex) *
    g_pplane_state.num_points;

  /* FIXME: when the bounds of the plane are changed,
     `fill_plane_data` tries to fill in more points than
     num_rows*num_columns. To account for that, I've allocated extra
     storage. However, the real fix is to get rid of the extra
     points. */
  points = malloc(2*g_pplane_state.points_size);

  /* Shaders and GLSL program */
  create_gl_resources();
  glUseProgram(g_pplane_state.shader_program);
  glBindVertexArray(g_pplane_state.vao);
  glBindBuffer(GL_ARRAY_BUFFER, g_pplane_state.vbo);

  glUniform2f(g_pplane_state.uniforms.scale, 1.0, 1.0);

  g_pplane_state.minX = -5.0;
  g_pplane_state.minY = -20.0;
  g_pplane_state.maxX = 20.0;
  g_pplane_state.maxY = 10.0;

  recompute_scale_and_translate();
  fill_plane_data();

  glBufferData(GL_ARRAY_BUFFER, g_pplane_state.points_size, points, GL_STATIC_DRAW);

  SDL_Event window_event;
  while (true) {
    nk_input_begin(ctx);
    if (SDL_PollEvent(&window_event)) {
      if (window_event.type == SDL_QUIT) break;
      nk_sdl_handle_event(&window_event);
    }
    nk_input_end(ctx);

    /* GUI */
    {
      struct nk_panel layout;
      if (nk_begin(ctx, &layout, "pplane", nk_rect(200, 200, 210, 350),
                   NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                   NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "x_min:", -100, &g_pplane_state.minX, 100, 10, 1);

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "x_max:", -100, &g_pplane_state.maxX, 100, 10, 1);


        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "y_min:", -100, &g_pplane_state.minY, 100, 10, 1);

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "y_max:", -100, &g_pplane_state.maxY, 100, 10, 1);

      }
      nk_end(ctx);
    }

    /* TODO: Check whether bounds have changed before doing this.  */
    recompute_scale_and_translate();
    fill_plane_data();

    fill_axes_data();
    set_mouse_position();

    /* Solver test */
    /* initial condition */
    vec2 init = {.x = 2, .y = 2};
    vec2 current = init;
    float dt = -0.01;
    for (int i = 400; i >= 0; i--) {
      vec2 current_canon = real_to_canonical_coords(current.x, current.y);
      g_pplane_state.solutions.solutions[0][i][0] = current_canon.x;
      g_pplane_state.solutions.solutions[0][i][1] = current_canon.y;
      current = rk4(current, dt);
    }

    dt = 0.01;
    current = init;
    for (int i = 400; i < 800; i++) {
      vec2 current_canon = real_to_canonical_coords(current.x, current.y);
      g_pplane_state.solutions.solutions[0][i][0] = current_canon.x;
      g_pplane_state.solutions.solutions[0][i][1] = current_canon.y;
      current = rk4(current, dt);
    }


    /* Draw */
    {float bg[4];
      nk_color_fv(bg, background);
      SDL_GetWindowSize(window, &win_width, &win_height);
      glViewport(0, 0, win_width, win_height);
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(bg[0], bg[1], bg[2], bg[3]);

      render();

      SDL_GL_SwapWindow(window);}

  }

  nk_sdl_shutdown();
  free(points);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
