#define GLSL(src) "#version 150 core\n" #src

const char* vertex_shader_src =
  GLSL(
       in vec2 pos;
       in vec2 dir;

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
         vDir = dir;
       }
       );

const char* fragment_shader_src =
  GLSL(
       out vec4 outColor;
       void main() {
         outColor = vec4(0.1, 1.0, 1.0, 1.0);
       }
       );

const char* geometry_shader_src =
  GLSL(
       layout(points) in;
       layout(line_strip, max_vertices=5) out;

       in vec2 vDir[];

       uniform vec2 scale_factor;

       float PI = 3.1415926;

       void main() {
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
