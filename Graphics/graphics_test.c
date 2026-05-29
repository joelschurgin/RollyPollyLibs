#include "graphics.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include <GL/gl.h>
#include <EGL/egl.h>

#define EGL_STATUS AssertAlways

i32 main(i32 argc, u8 **argv) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    AssertAlways(display != EGL_NO_DISPLAY);

    EGLint egl_version_major, egl_version_minor;

    EGL_STATUS(eglInitialize(display, &egl_version_major, &egl_version_minor));
    AssertAlways(egl_version_major == 1 && egl_version_minor <= 5);

    EGL_STATUS(eglBindAPI(EGL_OPENGL_API));

    b32 debug_mode = 0;
#ifdef BUILD_DEBUG
    debug_mode = 1;
#endif
    EGLint options[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        debug_mode ? EGL_CONTEXT_OPENGL_DEBUG : EGL_NONE, EGL_TRUE,
        EGL_NONE,
    };
    EGLContext context = eglCreateContext(display, 0, EGL_NO_CONTEXT, options);
    EGL_STATUS(context != EGL_NO_CONTEXT);

    //EGL_STATUS(eglMakeCurrent(display, 0, 0, context));
    //glDrawBuffer(GL_BACK);

    //surface = eglCreateWindowSurface(display, , , NULL);
    //eglDestroyContext(display, context);
    eglTerminate(display);
    printf("Version %d.%d\n", egl_version_major, egl_version_minor);

    return 0;
}
