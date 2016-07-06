#include <stdbool.h>
#include <math.h>

#define GLEW_STATIC
#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "shaders.c"

static float points[200][4];

typedef struct __vec2 {
  float x, y;
} vec2;

vec2
foo(float x, float y) {
  vec2 result;
  result.x = x + y;
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


int main() {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_Window* window = SDL_CreateWindow("pplane", 100, 100, 800, 600, SDL_WINDOW_OPENGL);
  SDL_GLContext context = SDL_GL_CreateContext(window);

  glewExperimental = GL_TRUE;
  glewInit();

  /* Shaders and GLSL program */
  GLuint vertexShader = create_shader(GL_VERTEX_SHADER, vertex_shader_src);
  GLuint fragmentShader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
  GLuint geometryShader = create_shader(GL_GEOMETRY_SHADER, geometry_shader_src);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glAttachShader(shaderProgram, geometryShader);
  glBindFragDataLocation(shaderProgram, 0, "outColor");
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

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
  GLint posAttrib = glGetAttribLocation(shaderProgram, "pos");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);

  GLint dirAttrib = glGetAttribLocation(shaderProgram, "dir");
  glEnableVertexAttribArray(dirAttrib);
  glVertexAttribPointer(dirAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),
                        (void*)(2*sizeof(float)));

  GLint scaleUniform = glGetUniformLocation(shaderProgram, "scale_factor");
  glUniform2f(scaleUniform, 1.0, 1.0);

  vec2 start = {-0.5, -0.5};

  int num_rows = 10;
  int num_columns = 10;

  for (int j = 0; j < num_rows; j++) {
    for (int i = 0; i < num_columns; i++) {
      vec2 arrow = unit_vector(foo(start.x + ((float)i/num_rows),
                                   start.y + ((float)j/num_columns)));

      int index = num_rows*i + j;

      points[index][0] = start.x + ((float)i/num_rows);
      points[index][1] = start.y + ((float)j/num_columns);

      points[index][2] = arrow.x * 0.05;
      points[index][3] = arrow.y * 0.05;
    }
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

  SDL_Event windowEvent;
  while (true) {
    if (SDL_PollEvent(&windowEvent)) {
      if (windowEvent.type == SDL_QUIT) break;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_POINTS, 0, 800);

    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(context);
  SDL_Quit();
  return 0;
}
