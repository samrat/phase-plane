#include <stdbool.h>

#define GLEW_STATIC
#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>


#define GLSL(src) "#version 150 core\n" #src

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


const char* vertex_shader_src =
  GLSL(
       in vec2 pos;
       in vec3 color;
       in vec2 dir;

       out vec3 vColor;
       out vec2 vDir;

       uniform vec2 scale_factor;

       mat4 scale(float x, float y) {
         return mat4(x, 0.0, 0.0, 0.0,
                     0.0, y, 0.0, 0.0,
                     0.0, 0.0, 1.0, 0.0,
                     0.0, 0.0, 0.0, 1.0);
       }

       void main() {
         gl_Position = scale(scale_factor.x, scale_factor.y) * vec4(pos, 0.0, 1.0);
         vColor = color;
         vDir = dir;
       }
       );

const char* fragment_shader_src =
  GLSL(
       in vec3 fColor;
       out vec4 outColor;

       void main() {
         outColor = vec4(fColor, 1.0);
       }
       );

const char* geometry_shader_src =
  GLSL(
       layout(points) in;
       layout(line_strip, max_vertices=5) out;

       in vec3 vColor[];
       in vec2 vDir[];
       out vec3 fColor;

       uniform vec2 scale_factor;

       float PI = 3.1415926;

       void main() {
         fColor = vColor[0];    /* point has only one vertex */

         gl_Position = gl_in[0].gl_Position;
         EmitVertex();

         gl_Position = gl_in[0].gl_Position + vec4(vDir[0], 0.0, 0.0);
         EmitVertex();

         /* Arrowhead */
         float ang = (PI / 6.0) - atan(vDir[0].y, vDir[0].x);

         gl_Position = gl_in[0].gl_Position + (vec4(vDir[0], 0.0, 0.0) +
                                               vec4(-cos(ang)*0.04, sin(ang)*0.04, 0.0, 0.0));
         EmitVertex();

         gl_Position = gl_in[0].gl_Position + vec4(vDir[0], 0.0, 0.0);
         EmitVertex();

         ang = -(PI / 6.0) - atan(vDir[0].y, vDir[0].x);

         gl_Position = gl_in[0].gl_Position + (vec4(vDir[0], 0.0, 0.0) +
                                               vec4(-cos(ang)*0.04, sin(ang)*0.04, 0.0, 0.0));
         EmitVertex();

         EndPrimitive();
       }
       );

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

  /* Buffer data */
  GLuint vbo;
  glGenBuffers(1, &vbo);

  float points[] = {
    -0.45f,  0.45f,  1.0f, 0.0f, 0.0f,  0.1, 0.0, // Red point
    0.45f,  0.45f,   0.0f, 1.0f, 0.0f,  -0.1, 0.1,// Green point
    0.45f, -0.45f,   0.0f, 0.0f, 1.0f,  0.0, 0.2, // Blue point
    -0.45f, -0.45f,  1.0f, 1.0f, 0.0f,  0.1, 0.1 // Yellow point
  };

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

  // Create VAO
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Specify layout of point data
  GLint posAttrib = glGetAttribLocation(shaderProgram, "pos");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), 0);

  GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
  glEnableVertexAttribArray(colAttrib);
  glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float),
                        (void*)(2*sizeof(float)));

  GLint dirAttrib = glGetAttribLocation(shaderProgram, "dir");
  glEnableVertexAttribArray(dirAttrib);
  glVertexAttribPointer(dirAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float),
                        (void*)(5*sizeof(float)));

  GLint scaleUniform = glGetUniformLocation(shaderProgram, "scale_factor");
  glUniform2f(scaleUniform, 1.0, 1.0);

  SDL_Event windowEvent;
  while (true) {
    if (SDL_PollEvent(&windowEvent)) {
      if (windowEvent.type == SDL_QUIT) break;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_POINTS, 0, 4);

    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(context);
  SDL_Quit();
  return 0;
}
