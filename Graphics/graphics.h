#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include <wayland-egl.h>

#define glActiveTexture glActiveTexture__static
#include <GL/gl.h>
#include <EGL/egl.h>
#undef glActiveTexture

#include "base.h"
//#include "graphics_font.h"

extern Arena* graphics_arena;

typedef void (*graphics_draw_func_t)(void* data);
typedef struct Graphics_Shader Graphics_Shader;
typedef struct Graphics_Rect Graphics_Rect;
typedef struct Graphics_ImageRect Graphics_ImageRect;

typedef struct Graphics_Window Graphics_Window;

typedef struct {
    f32 r, g, b, a;
} Graphics_Color;
#define Graphics_Color(red, green, blue, alpha) (Graphics_Color){ .r = (red), .g = (green), .b = (blue), .a = (alpha) }

void                    graphics_init();
Graphics_Window*        graphics_window_create();
void                    graphics_window_close(Graphics_Window* window);
b32                     graphics_poll_events(Graphics_Window* window);

void                    graphics_window_dimensions(Graphics_Window* window, u32* width, u32* height);

void                    graphics_set_draw_callback(Graphics_Window* window, graphics_draw_func_t draw_func, void* data);

Graphics_Rect           graphics_rect_create();
void                    graphics_rect_fill(Graphics_Window* window, f32 x, f32 y, f32 w, f32 h, f32 radius, f32 border_thickness, Graphics_Color fill_color, Graphics_Color border_color);

Graphics_ImageRect      graphics_image_rect_create();
void                    graphics_image_rect_draw(Graphics_Window* window, f32 rect_x, f32 rect_y, f32 rect_w, f32 rect_h);

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_FRAMEBUFFER                    0x8D40
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_TEXTURE_MAX_LEVEL              0x813D

#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9

#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_COLOR_ATTACHMENT2              0x8CE2
#define GL_COLOR_ATTACHMENT3              0x8CE3
#define GL_COLOR_ATTACHMENT4              0x8CE4
#define GL_COLOR_ATTACHMENT5              0x8CE5
#define GL_COLOR_ATTACHMENT6              0x8CE6
#define GL_COLOR_ATTACHMENT7              0x8CE7
#define GL_COLOR_ATTACHMENT8              0x8CE8
#define GL_COLOR_ATTACHMENT9              0x8CE9
#define GL_COLOR_ATTACHMENT10             0x8CEA
#define GL_COLOR_ATTACHMENT11             0x8CEB
#define GL_COLOR_ATTACHMENT12             0x8CEC
#define GL_COLOR_ATTACHMENT13             0x8CED
#define GL_COLOR_ATTACHMENT14             0x8CEE
#define GL_COLOR_ATTACHMENT15             0x8CEF
#define GL_COLOR_ATTACHMENT16             0x8CF0
#define GL_COLOR_ATTACHMENT17             0x8CF1
#define GL_COLOR_ATTACHMENT18             0x8CF2
#define GL_COLOR_ATTACHMENT19             0x8CF3
#define GL_COLOR_ATTACHMENT20             0x8CF4
#define GL_COLOR_ATTACHMENT21             0x8CF5
#define GL_COLOR_ATTACHMENT22             0x8CF6
#define GL_COLOR_ATTACHMENT23             0x8CF7
#define GL_COLOR_ATTACHMENT24             0x8CF8
#define GL_COLOR_ATTACHMENT25             0x8CF9
#define GL_COLOR_ATTACHMENT26             0x8CFA
#define GL_COLOR_ATTACHMENT27             0x8CFB
#define GL_COLOR_ATTACHMENT28             0x8CFC
#define GL_COLOR_ATTACHMENT29             0x8CFD
#define GL_COLOR_ATTACHMENT30             0x8CFE
#define GL_COLOR_ATTACHMENT31             0x8CFF

#define GL_R8                             0x8229

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_INFO_LOG_LENGTH                0x8B84

#define GL_TEXTURE_2D_ARRAY               0x8C1A

#define GL_COMPILE_STATUS                 0x8B81

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF

#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242


#define Graphics_OpenGL_ProcedureXList \
X(glGenBuffers, void, (GLsizei n, GLuint *buffers))\
X(glBindBuffer, void, (GLenum target, GLuint buffer))\
X(glDeleteBuffers, void, (GLsizei n, GLuint *buffers))\
X(glGenVertexArrays, void, (GLsizei n, GLuint *arrays))\
X(glBindVertexArray, void, (GLuint array))\
X(glGenFramebuffers, void, (GLsizei n, GLuint *fbos))\
X(glDeleteFramebuffers, void, (GLsizei n, GLuint *fbos))\
X(glCreateProgram, GLuint, (void))\
X(glCreateShader, GLuint, (GLenum type))\
X(glShaderSource, void, (GLuint shader, GLsizei count, char **string, GLint *length))\
X(glCompileShader, void, (GLuint shader))\
X(glGetShaderiv, void, (GLuint shader, GLenum pname, GLint *params))\
X(glGetShaderInfoLog, void, (GLuint shader, GLsizei bufSize, GLsizei *length, char *infoLog))\
X(glGetProgramiv, void, (GLuint program, GLenum pname, GLint *params))\
X(glGetProgramInfoLog, void, (GLuint program, GLsizei bufSize, GLsizei *length, char *infoLog))\
X(glAttachShader, void, (GLuint program, GLuint shader))\
X(glDetachShader, void, (GLuint program, GLuint shader))\
X(glLinkProgram, void, (GLuint program))\
X(glValidateProgram, void, (GLuint program))\
X(glDeleteShader, void, (GLuint shader))\
X(glUseProgram, void, (GLuint program))\
X(glGetUniformLocation, GLint, (GLuint program, char *name))\
X(glGetAttribLocation, GLint, (GLuint program, char *name))\
X(glEnableVertexAttribArray, void, (GLuint index))\
X(glVertexAttribPointer, void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))\
X(glBufferData, void, (GLenum target, ptrdiff_t size, void *data, GLenum usage))\
X(glBufferSubData, void, (GLenum target, ptrdiff_t offset, ptrdiff_t size, const void *data))\
X(glBlendFuncSeparate, void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha))\
X(glUniform1f, void, (GLint location, GLfloat v0))\
X(glUniform2f, void, (GLint location, GLfloat v0, GLfloat v1))\
X(glUniform3f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2))\
X(glUniform4f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))\
X(glUniform4fv, void, (GLint location, GLsizei count, const GLfloat *value))\
X(glUniformMatrix3fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))\
X(glUniformMatrix4fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))\
X(glUniform1i, void, (GLint location, GLint v0))\
X(glGenerateMipmap, void, (GLenum target))\
X(glBindAttribLocation, void, (GLuint programObj, GLuint index, char *name))\
X(glBindFragDataLocation, void, (GLuint program, GLuint color, char *name))\
X(glBindFramebuffer, void, (GLenum target, GLuint fbo))\
X(glActiveTexture, void, (GLenum texture))


#define X(name, r, p) typedef r name##_FunctionType p;
Graphics_OpenGL_ProcedureXList
#undef X
#define X(name, r, p) global name##_FunctionType *name = 0;
Graphics_OpenGL_ProcedureXList
#undef X
