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

#include "pplane.h"
#include "shaders.c"
#include "solver.c"
#include "interpreter.c"


#define WIDTH 800
#define HEIGHT 600

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

static const int num_rows = 20;
static const int num_columns = 20;

vec2
diffeq_system(vec2 current) {
  vec2 result;

  env.x = current.x;
  env.y = current.y;
  look = &xtest[0];

  result.x = expression();

  look = &ytest[0];
  result.y = expression();

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
create_axes_gl_state(pplane_state_t *pplane_state) {
  gl_state_t *gl_state = pplane_state->gl_state;
  gl_state->axes.vertex_shader = create_shader(GL_VERTEX_SHADER, axes_vertex_shader_src);
  gl_state->axes.fragment_shader = create_shader(GL_FRAGMENT_SHADER, axes_fragment_shader_src);
  gl_state->axes.shader_program = glCreateProgram();
  glAttachShader(gl_state->axes.shader_program, gl_state->axes.vertex_shader);
  glAttachShader(gl_state->axes.shader_program, gl_state->axes.fragment_shader);
  glBindFragDataLocation(gl_state->axes.shader_program, 0, "outColor");
  glLinkProgram(gl_state->axes.shader_program);
  glUseProgram(gl_state->axes.shader_program);

  glGenVertexArrays(1, &gl_state->axes.vao);
  glBindVertexArray(gl_state->axes.vao);

  glGenBuffers(1, &gl_state->axes.vbo);

  glBindBuffer(GL_ARRAY_BUFFER, gl_state->axes.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl_state->axes.endpoints),
               gl_state->axes.endpoints, GL_STATIC_DRAW);

  gl_state->axes.attributes.pos =
    glGetAttribLocation(gl_state->axes.shader_program, "pos");
  glEnableVertexAttribArray(gl_state->axes.attributes.pos);
  glVertexAttribPointer(gl_state->axes.attributes.pos, 2, GL_FLOAT,
                        GL_FALSE, 0, 0);
  gl_state->axes.uniforms.scale =
    glGetUniformLocation(gl_state->axes.shader_program, "scale_factor");

  return 0;
}

int
create_solutions_gl_state(pplane_state_t *pplane_state) {
  gl_state_t *gl_state = pplane_state->gl_state;
  gl_state->solutions.vertex_shader = create_shader(GL_VERTEX_SHADER, axes_vertex_shader_src);
  gl_state->solutions.fragment_shader = create_shader(GL_FRAGMENT_SHADER, axes_fragment_shader_src);
  gl_state->solutions.shader_program = glCreateProgram();
  glAttachShader(gl_state->solutions.shader_program, gl_state->solutions.vertex_shader);
  glAttachShader(gl_state->solutions.shader_program, gl_state->solutions.fragment_shader);
  glBindFragDataLocation(gl_state->solutions.shader_program, 0, "outColor");
  glLinkProgram(gl_state->solutions.shader_program);
  glUseProgram(gl_state->solutions.shader_program);

  glGenVertexArrays(1, &gl_state->solutions.vao);
  glBindVertexArray(gl_state->solutions.vao);

  glGenBuffers(1, &gl_state->solutions.vbo);

  glBindBuffer(GL_ARRAY_BUFFER, gl_state->solutions.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl_state->solutions.solutions),
               gl_state->solutions.solutions, GL_STATIC_DRAW);

  gl_state->solutions.attributes.pos =
    glGetAttribLocation(gl_state->solutions.shader_program, "pos");
  glEnableVertexAttribArray(gl_state->solutions.attributes.pos);
  glVertexAttribPointer(gl_state->solutions.attributes.pos, 2, GL_FLOAT,
                        GL_FALSE, 0, 0);
  gl_state->solutions.uniforms.scale =
    glGetUniformLocation(gl_state->solutions.shader_program, "scale_factor");

  return 0;
}

int
create_plane_gl_resources(pplane_state_t *pplane_state) {
  gl_state_t *gl_state = pplane_state->gl_state;
  gl_state->plane.vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_src);
  gl_state->plane.fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
  gl_state->plane.geometry_shader = create_shader(GL_GEOMETRY_SHADER, geometry_shader_src);

  gl_state->plane.shader_program = glCreateProgram();
  glAttachShader(gl_state->plane.shader_program, gl_state->plane.vertex_shader);
  glAttachShader(gl_state->plane.shader_program, gl_state->plane.fragment_shader);
  glAttachShader(gl_state->plane.shader_program, gl_state->plane.geometry_shader);
  glBindFragDataLocation(gl_state->plane.shader_program, 0, "outColor");
  glLinkProgram(gl_state->plane.shader_program);
  glUseProgram(gl_state->plane.shader_program);

  // Create VAO
  glGenVertexArrays(1, &gl_state->plane.vao);
  glBindVertexArray(gl_state->plane.vao);

  /* Buffer data */
  glGenBuffers(1, &gl_state->plane.vbo);

  glBindBuffer(GL_ARRAY_BUFFER, gl_state->plane.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl_state->plane.points), gl_state->plane.points, GL_STATIC_DRAW);

  // Specify layout of point data
  gl_state->plane.attributes.pos =
    glGetAttribLocation(gl_state->plane.shader_program, "pos");
  glEnableVertexAttribArray(gl_state->plane.attributes.pos);
  glVertexAttribPointer(gl_state->plane.attributes.pos, 2, GL_FLOAT,
                        GL_FALSE, 4*sizeof(float), 0);

  gl_state->plane.attributes.dir =
    glGetAttribLocation(gl_state->plane.shader_program, "dir");
  glEnableVertexAttribArray(gl_state->plane.attributes.dir);
  glVertexAttribPointer(gl_state->plane.attributes.dir, 2, GL_FLOAT,
                        GL_FALSE, 4*sizeof(float),
                        (void*)(2*sizeof(float)));

  gl_state->plane.uniforms.scale =
    glGetUniformLocation(gl_state->plane.shader_program, "scale_factor");

  return 0;
}

int
create_gl_resources(pplane_state_t *pplane_state) {
  /* Plane */
  create_plane_gl_resources(pplane_state);
  /* Axes */
  create_axes_gl_state(pplane_state);
  /* Solutions */
  create_solutions_gl_state(pplane_state);

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
real_to_canonical_coords(pplane_state_t *pplane_state, float x, float y) {
  vec2 result;
  result.x = (x * pplane_state->scaleX) - pplane_state->translateX;
  result.y = (y * pplane_state->scaleY) - pplane_state->translateY;

  return result;
}

vec2
canonical_to_real_coords(pplane_state_t *pplane_state, float x, float y) {
  vec2 result;
  result.x = (x + pplane_state->translateX) / pplane_state->scaleX;
  result.y = (y + pplane_state->translateY) / pplane_state->scaleY;

  return result;
}

static void
render(pplane_state_t *pplane_state) {
  gl_state_t *gl_state = pplane_state->gl_state;
  glUseProgram(gl_state->plane.shader_program);
  glBindVertexArray(gl_state->plane.vao);
  glBindBuffer(GL_ARRAY_BUFFER, gl_state->plane.vbo);

  /* TODO: Can I update just the data at points[num_points-1] ? */
  glBufferSubData(GL_ARRAY_BUFFER, 0,
                  pplane_state->points_size, gl_state->plane.points);
  glDrawArrays(GL_POINTS, 0, pplane_state->num_points);

  /* Axes */
  glUseProgram(gl_state->axes.shader_program);
  glBindVertexArray(gl_state->axes.vao);
  glBindBuffer(GL_ARRAY_BUFFER, gl_state->axes.vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gl_state->axes.endpoints), gl_state->axes.endpoints);
  glUniform2f(gl_state->axes.uniforms.scale, 1.0, 1.0);

  glDrawArrays(GL_LINE_STRIP, 0, 6);

  /* Solutions */
  glUseProgram(gl_state->solutions.shader_program);
  glBindVertexArray(gl_state->solutions.vao);

  glBindBuffer(GL_ARRAY_BUFFER, gl_state->solutions.vbo);

  for (int i = 0; i < gl_state->solutions.num_solutions; i++) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gl_state->solutions.solutions[i]), gl_state->solutions.solutions[i]);
    glUniform2f(gl_state->solutions.uniforms.scale, 1.0, 1.0);
    glDrawArrays(GL_LINE_STRIP, 0, 2*HALF_NUM_STEPS_PER_SOLUTION);
  }

  nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
}

static void
recompute_scale_and_translate(pplane_state_t *pplane_state) {
  pplane_state->scaleX = 2.0f / (pplane_state->maxX - pplane_state->minX);
  pplane_state->scaleY = 2.0f / (pplane_state->maxY - pplane_state->minY);

  pplane_state->translateX = 1.0f - (-2*pplane_state->minX /
                                      (pplane_state->maxX - pplane_state->minX));
  pplane_state->translateY = 1.0f - (-2*pplane_state->minY /
                                      (pplane_state->maxY - pplane_state->minY));

}

static void
fill_plane_data(pplane_state_t *pplane_state) {
  vec2 min = canonical_to_real_coords(pplane_state, -1.0, -1.0);
  vec2 max = canonical_to_real_coords(pplane_state, 1.0, 1.0);

  float stepX = (max.x - min.x) / num_rows;
  float stepY = (max.y - min.y) / num_columns;

  point_vertex *points = pplane_state->gl_state->plane.points;

  int index = 0;
  for (float i = min.x;
       i < max.x;
       i += stepX) {
    for (float j = min.y;
         j < max.y;
         j += stepY) {
      vec2 v = {.x = i, .y = j };
      vec2 arrow = unit_vector(diffeq_system(v));
      vec2 canon_coords = real_to_canonical_coords(pplane_state, i, j);

      points[index].x = canon_coords.x;
      points[index].y = canon_coords.y;

      points[index].dirX = arrow.x * 0.05;
      points[index].dirY = arrow.y * 0.05;

      index += 1;
    }
  }
}

static void
fill_axes_data(pplane_state_t *pplane_state) {
  gl_state_t *gl_state = pplane_state->gl_state;
  vec2 left = real_to_canonical_coords(pplane_state,
                                       pplane_state->minX, 0);
  gl_state->axes.endpoints[0][0] = left.x;
  gl_state->axes.endpoints[0][1] = left.y;

  vec2 center = real_to_canonical_coords(pplane_state,
                                         0, 0);
  gl_state->axes.endpoints[1][0] = center.x;
  gl_state->axes.endpoints[1][1] = center.y;

  vec2 top = real_to_canonical_coords(pplane_state,
                                      0, pplane_state->maxY);
  gl_state->axes.endpoints[2][0] = top.x;
  gl_state->axes.endpoints[2][1] = top.y;

  vec2 bottom = real_to_canonical_coords(pplane_state,
                                         0, pplane_state->minY);
  gl_state->axes.endpoints[3][0] = bottom.x;
  gl_state->axes.endpoints[3][1] = bottom.y;

  gl_state->axes.endpoints[4][0] = center.x;
  gl_state->axes.endpoints[4][1] = center.y;

  vec2 right = real_to_canonical_coords(pplane_state,
                                        pplane_state->maxX, 0);
  gl_state->axes.endpoints[5][0] = right.x;
  gl_state->axes.endpoints[5][1] = right.y;
}

static void
set_mouse_position(pplane_state_t *pplane_state) {
  vec2 m = canonical_mouse_pos();
  vec2 real_m = canonical_to_real_coords(pplane_state,
                                         m.x, m.y);
  vec2 arrow = unit_vector(diffeq_system(real_m));
  point_vertex *points = pplane_state->gl_state->plane.points;

  points[pplane_state->num_points-1].x = m.x;
  points[pplane_state->num_points-1].y = m.y;
  points[pplane_state->num_points-1].dirX = arrow.x;
  points[pplane_state->num_points-1].dirY = arrow.y;
}

static void
handle_event(pplane_state_t *pplane_state, SDL_Event *event) {
  gl_state_t *gl_state = pplane_state->gl_state;
  /* TODO: ignore mouse clicks on Nuklear GUI */
  if (event->type == SDL_MOUSEBUTTONDOWN) {
    int solutions_idx = gl_state->solutions.num_solutions;

    vec2 canon_init = canonical_mouse_pos();
    vec2 real_init = canonical_to_real_coords(pplane_state,
                                              canon_init.x,
                                              canon_init.y);
    gl_state->solutions.init[solutions_idx][0] = real_init.x;
    gl_state->solutions.init[solutions_idx][1] = real_init.y;

    gl_state->solutions.num_solutions += 1;
    gl_state->solutions.recompute_solutions = true;
  }
}

int main(int argc, char *argv[]) {
  gl_state_t gl_state;
  pplane_state_t pplane_state;
  pplane_state.gl_state = &gl_state;

  env.x = 2.3;
  env.y = 1.0;
  printf("%f\n", expression());

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

  gl_state.solutions.num_solutions = 0;


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
  pplane_state.num_points = num_rows*num_columns + 1;
  pplane_state.points_size = sizeof(point_vertex) *
    pplane_state.num_points;

  /* FIXME: when the bounds of the plane are changed,
     `fill_plane_data` tries to fill in more points than
     num_rows*num_columns. To account for that, I've allocated extra
     storage. However, the real fix is to get rid of the extra
     points. */
  gl_state.plane.points = malloc(2*pplane_state.points_size);

  /* Shaders and GLSL program */
  create_gl_resources(&pplane_state);
  glUseProgram(gl_state.plane.shader_program);
  glBindVertexArray(gl_state.plane.vao);
  glBindBuffer(GL_ARRAY_BUFFER, gl_state.plane.vbo);

  glUniform2f(gl_state.plane.uniforms.scale, 1.0, 1.0);

  pplane_state.minX = -5.0;
  pplane_state.minY = -20.0;
  pplane_state.maxX = 20.0;
  pplane_state.maxY = 10.0;

  recompute_scale_and_translate(&pplane_state);
  fill_plane_data(&pplane_state);

  glBufferData(GL_ARRAY_BUFFER, pplane_state.points_size,
               gl_state.plane.points, GL_STATIC_DRAW);

  SDL_Event window_event;
  while (true) {
    nk_input_begin(ctx);
    if (SDL_PollEvent(&window_event)) {
      if (window_event.type == SDL_QUIT) break;
      nk_sdl_handle_event(&window_event);
      handle_event(&pplane_state, &window_event);
    }
    nk_input_end(ctx);

    /* GUI */
    {
      struct nk_panel layout;
      if (nk_begin(ctx, &layout, "pplane", nk_rect(200, 200, 210, 350),
                   NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                   NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
        static float minX = -5.0;
        static float minY = -20.0;
        static float maxX = 20.0;
        static float maxY = 10.0;
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "x_min:", -100, &minX, 100, 10, 1);

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "x_max:", -100, &maxX, 100, 10, 1);


        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "y_min:", -100, &minY, 100, 10, 1);

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "y_max:", -100, &maxY, 100, 10, 1);

        if (nk_button_label(ctx, "Apply changes", NK_BUTTON_DEFAULT)) {
          gl_state.solutions.recompute_solutions = true;
          pplane_state.minX = minX;
          pplane_state.maxX = maxX;

          pplane_state.minY = minY;
          pplane_state.maxY = maxY;
        }

        if (nk_button_label(ctx, "Clear solutions", NK_BUTTON_DEFAULT)) {
          gl_state.solutions.num_solutions = 0;
        }

      }
      nk_end(ctx);

      /* Diffeq. System Editor */
      /* TODO: How do I show the default system? */
      struct nk_panel layout2;
      if (nk_begin(ctx, &layout2, "System", nk_rect(250, 200, 210, 200),
                   NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                   NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
        static int xlen = 5;
        static char xbuffer[128] = "x*x+y";
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, xbuffer, &xlen, 128, nk_filter_ascii);
        xbuffer[xlen] = 0;
        printf("x = %s\n", xtest);

        static int ylen = 3;
        static char ybuffer[128] = "x-y";

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, ybuffer, &ylen, 128, nk_filter_ascii);
        ybuffer[ylen] = 0;
        printf("y = %s\n", ytest);

        if (nk_button_label(ctx, "Apply", NK_BUTTON_DEFAULT)) {
          snprintf(xtest, 128, "%s", xbuffer);
          snprintf(ytest, 128, "%s", ybuffer);
        }
      }
      nk_end(ctx);
    }

    /* TODO: Check whether bounds have changed before doing this.  */
    recompute_scale_and_translate(&pplane_state);
    fill_plane_data(&pplane_state);

    fill_axes_data(&pplane_state);
    set_mouse_position(&pplane_state);

    /* Solver test */
    /* TODO- Add a check to re-fill solutions buffer only when
       necessary */
    if (gl_state.solutions.recompute_solutions) {
      for (int c = 0; c < gl_state.solutions.num_solutions; c++) {
        vec2 current;
        current.x = gl_state.solutions.init[c][0];
        current.y = gl_state.solutions.init[c][1];

        float dt = -SOLUTION_DT;
        for (int i = HALF_NUM_STEPS_PER_SOLUTION; i >= 0; i--) {
          vec2 current_canon = real_to_canonical_coords(&pplane_state,
                                                        current.x, current.y);
          gl_state.solutions.solutions[c][i][0] = current_canon.x;
          gl_state.solutions.solutions[c][i][1] = current_canon.y;
          current = rk4(current, dt);
        }

        dt = SOLUTION_DT;
        current.x = gl_state.solutions.init[c][0];
        current.y = gl_state.solutions.init[c][1];

        for (int i = HALF_NUM_STEPS_PER_SOLUTION; i < 2*HALF_NUM_STEPS_PER_SOLUTION; i++) {
          vec2 current_canon = real_to_canonical_coords(&pplane_state,
                                                        current.x, current.y);
          gl_state.solutions.solutions[c][i][0] = current_canon.x;
          gl_state.solutions.solutions[c][i][1] = current_canon.y;
          current = rk4(current, dt);
        }
      }
      gl_state.solutions.recompute_solutions = false;
    }

    /* Draw */
    {float bg[4];
      nk_color_fv(bg, background);
      SDL_GetWindowSize(window, &win_width, &win_height);
      glViewport(0, 0, win_width, win_height);
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(bg[0], bg[1], bg[2], bg[3]);

      render(&pplane_state);

      SDL_GL_SwapWindow(window);}

  }

  nk_sdl_shutdown();
  free(gl_state.plane.points);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
