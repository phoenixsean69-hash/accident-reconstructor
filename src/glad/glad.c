#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glad.h"

/* ROADSAFE_GL_LOADER_V1 */

static void* get_proc(const char *name);

PFNGLCLEARPROC glad_glClear = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;
PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = NULL;
PFNGLCULLFACEPROC glad_glCullFace = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLPOLYGONMODEPROC glad_glPolygonMode = NULL;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = NULL;

PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLUNIFORM1FPROC glad_glUniform1f = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLUNIFORM3FPROC glad_glUniform3f = NULL;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = NULL;

PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;

PFNGLGETSTRINGPROC glad_glGetString = NULL;
PFNGLGETSTRINGIPROC glad_glGetStringi = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;

PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = NULL;

PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers = NULL;
PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D = NULL;

PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers = NULL;
PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers = NULL;
PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer = NULL;

#if defined(_WIN32)
#include <windows.h>
static HMODULE opengl32 = NULL;

static void* get_proc(const char *name) {
    void *p = (void*)wglGetProcAddress(name);

    if(p == 0 ||
       p == (void*)0x1 ||
       p == (void*)0x2 ||
       p == (void*)0x3 ||
       p == (void*)-1)
    {
        if(!opengl32)
            opengl32 = LoadLibraryA("opengl32.dll");

        p = (void*)GetProcAddress(opengl32, name);
    }

    return p;
}
#else
#include <dlfcn.h>
static void* libgl = NULL;

static void* get_proc(const char *name) {
    if(!libgl)
        libgl = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);

    void *p = dlsym(libgl, name);

    if(!p)
        p = dlsym(RTLD_DEFAULT, name);

    return p;
}
#endif

#define LOAD_GL(name, type) \
    glad_##name = (type)get_proc(#name)

static int load_gl_version(void) {
    LOAD_GL(glClear, PFNGLCLEARPROC);
    LOAD_GL(glClearColor, PFNGLCLEARCOLORPROC);
    LOAD_GL(glViewport, PFNGLVIEWPORTPROC);
    LOAD_GL(glEnable, PFNGLENABLEPROC);
    LOAD_GL(glDisable, PFNGLDISABLEPROC);
    LOAD_GL(glDepthFunc, PFNGLDEPTHFUNCPROC);
    LOAD_GL(glCullFace, PFNGLCULLFACEPROC);
    LOAD_GL(glBlendFunc, PFNGLBLENDFUNCPROC);
    LOAD_GL(glPolygonMode, PFNGLPOLYGONMODEPROC);
    LOAD_GL(glPixelStorei, PFNGLPIXELSTOREIPROC);

    LOAD_GL(glCreateShader, PFNGLCREATESHADERPROC);
    LOAD_GL(glDeleteShader, PFNGLDELETESHADERPROC);
    LOAD_GL(glShaderSource, PFNGLSHADERSOURCEPROC);
    LOAD_GL(glCompileShader, PFNGLCOMPILESHADERPROC);
    LOAD_GL(glGetShaderiv, PFNGLGETSHADERIVPROC);
    LOAD_GL(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
    LOAD_GL(glCreateProgram, PFNGLCREATEPROGRAMPROC);
    LOAD_GL(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
    LOAD_GL(glAttachShader, PFNGLATTACHSHADERPROC);
    LOAD_GL(glLinkProgram, PFNGLLINKPROGRAMPROC);
    LOAD_GL(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
    LOAD_GL(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    LOAD_GL(glUseProgram, PFNGLUSEPROGRAMPROC);
    LOAD_GL(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    LOAD_GL(glUniform1f, PFNGLUNIFORM1FPROC);
    LOAD_GL(glUniform1i, PFNGLUNIFORM1IPROC);
    LOAD_GL(glUniform3f, PFNGLUNIFORM3FPROC);
    LOAD_GL(glUniform4fv, PFNGLUNIFORM4FVPROC);
    LOAD_GL(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);

    LOAD_GL(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
    LOAD_GL(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC);
    LOAD_GL(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
    LOAD_GL(glGenBuffers, PFNGLGENBUFFERSPROC);
    LOAD_GL(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
    LOAD_GL(glBindBuffer, PFNGLBINDBUFFERPROC);
    LOAD_GL(glBufferData, PFNGLBUFFERDATAPROC);
    LOAD_GL(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    LOAD_GL(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    LOAD_GL(glDrawArrays, PFNGLDRAWARRAYSPROC);
    LOAD_GL(glDrawElements, PFNGLDRAWELEMENTSPROC);

    LOAD_GL(glGetString, PFNGLGETSTRINGPROC);
    LOAD_GL(glGetStringi, PFNGLGETSTRINGIPROC);
    LOAD_GL(glGetIntegerv, PFNGLGETINTEGERVPROC);

    LOAD_GL(glGenTextures, PFNGLGENTEXTURESPROC);
    LOAD_GL(glDeleteTextures, PFNGLDELETETEXTURESPROC);
    LOAD_GL(glBindTexture, PFNGLBINDTEXTUREPROC);
    LOAD_GL(glActiveTexture, PFNGLACTIVETEXTUREPROC);
    LOAD_GL(glTexParameteri, PFNGLTEXPARAMETERIPROC);
    LOAD_GL(glTexImage2D, PFNGLTEXIMAGE2DPROC);
    LOAD_GL(glGenerateMipmap, PFNGLGENERATEMIPMAPPROC);

    LOAD_GL(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC);
    LOAD_GL(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC);
    LOAD_GL(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC);
    LOAD_GL(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC);
    LOAD_GL(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC);

    LOAD_GL(glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC);
    LOAD_GL(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC);
    LOAD_GL(glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC);
    LOAD_GL(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEPROC);
    LOAD_GL(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC);

    return
        glad_glCreateShader &&
        glad_glCreateProgram &&
        glad_glGenVertexArrays &&
        glad_glGenBuffers &&
        glad_glGenTextures &&
        glad_glDrawElements &&
        glad_glGenFramebuffers &&
        glad_glGenRenderbuffers &&
        glad_glGetUniformLocation &&
        glad_glUniformMatrix4fv;
}

#undef LOAD_GL

int gladLoadGLLoader(GLADloadproc load) {
    (void)load;
    return load_gl_version();
}
