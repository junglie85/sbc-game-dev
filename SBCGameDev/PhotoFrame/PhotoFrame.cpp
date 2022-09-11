/* PhotoFrame
code adapted from from OpenGL® ES 2.0 Programming Guide
and code snippets from RPI Forum to set up Dispmanx
*/

#include "MyFiles.h"

#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <X11/Xlib.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

char* Images[] = { // place your pics in the Assets folder, add them to the solution if you wish
    (char*)"Assets//SBCs.jpg", (char*)"Assets//lenna.png"
};
// notice that if you want to comment a define use the /* */ comment format
#define NUMBEROFPICS 2 /* how many entries have we */

typedef struct {
    // save a Handle to a program object
    GLuint programObject;
    // Attribute locations
    GLint positionLoc;
    GLint texCoordLoc;

    // Sampler location
    GLint samplerLoc;

    // Texture handle
    GLuint textureId;
} UserData;

typedef struct Target_State {
    uint32_t width;
    uint32_t height;

    Display* xDisplay;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLNativeWindowType nativeWindow;

    UserData* user_data;
    void (*draw_func)(struct Target_State*);

} Target_State;

Target_State state; // this is our Targets render data

Target_State* p_state = &state; // this is a useful pointer to our target state

static const EGLint attribute_list[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };

static const EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

/*
 Create a texture with width and height
*/
GLuint CreateTexture2D(int width, int height, char* TheData)
{
    // Texture handle
    GLuint textureId;
    // Set the alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Generate a texture object
    glGenTextures(1, &textureId);
    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, textureId);
    // set it up
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, TheData);

    if (glGetError() != GL_NO_ERROR)
        printf("Oh bugger"); // its a good idea to test for errors.

    // Set the filtering mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (glGetError() == GL_NO_ERROR)
        return textureId;
    printf("Oh bugger");

    return textureId;
}

/*
 Now we have be able to create a shader object, pass the shader source
 and them compile the shader.
*/

GLuint LoadShader(GLenum type, const char* shaderSrc)
{
    // 1st create the shader object
    GLuint TheShader = glCreateShader(type);

    if (TheShader == 0)
        return 0; // can't allocate so stop.
    // pass the shader source
    glShaderSource(TheShader, 1, &shaderSrc, NULL);
    // Compile the shader
    glCompileShader(TheShader);

    GLint IsItCompiled;

    // After the compile we need to check the status and report any errors
    glGetShaderiv(TheShader, GL_COMPILE_STATUS, &IsItCompiled);
    if (!IsItCompiled) {
        GLint RetinfoLen = 0;
        glGetShaderiv(TheShader, GL_INFO_LOG_LENGTH, &RetinfoLen);
        if (RetinfoLen > 1) { // standard output for errors
            char* infoLog = (char*)malloc(sizeof(char) * RetinfoLen);
            glGetShaderInfoLog(TheShader, RetinfoLen, NULL, infoLog);
            fprintf(stderr, "Error compiling this shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(TheShader);
        return 0;
    }
    return TheShader;
}

///
// Initialize the shader and program object
//
int Init(Target_State* p_state)
{
    p_state->user_data = (UserData*)malloc(sizeof(UserData));

    GLbyte vShaderStr[] = "#version 300 es\n"
                          "layout (location = 0) in vec4 a_position;\n"
                          "layout (location = 1) in vec2 a_texCoord;\n"
                          "out vec2 v_texCoord;\n"
                          "void main()\n"
                          "{\n"
                          "    gl_Position = a_position;\n"
                          "    v_texCoord = a_texCoord;\n"
                          "}\n";

    GLbyte fShaderStr[] = "#version 300 es\n"
                          "precision mediump float;\n"
                          "in vec2 v_texCoord;\n"
                          "out vec4 fragColor;\n"
                          "uniform sampler2D s_texture;\n"
                          "void main()\n"
                          "{\n"
                          "    fragColor = texture2D(s_texture, v_texCoord);\n"
                          "}\n";

    GLuint programObject, vertexShader, fragmentShader; // we need some variables

    // Load and compile the vertex/fragment shaders
    vertexShader = LoadShader(GL_VERTEX_SHADER, (char*)vShaderStr);
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, (char*)fShaderStr);

    // Create the program object
    programObject = glCreateProgram();
    if (programObject == 0)
        return 0;

    // now we have teh V and F shaders  attach them to the progam object
    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    // Link the program
    glLinkProgram(programObject);
    // Check the link status
    GLint AreTheylinked;
    glGetProgramiv(programObject, GL_LINK_STATUS, &AreTheylinked);
    if (!AreTheylinked) {
        GLint RetinfoLen = 0;
        // check and report any errors
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &RetinfoLen);
        if (RetinfoLen > 1) {
            GLchar* infoLog = (GLchar*)malloc(sizeof(char) * RetinfoLen);
            glGetProgramInfoLog(programObject, RetinfoLen, NULL, infoLog);
            fprintf(stderr, "Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(programObject);
        return FALSE;
    }

    // Store the program object
    p_state->user_data->programObject = programObject;

    // Get the attribute locations
    p_state->user_data->positionLoc
        = glGetAttribLocation(p_state->user_data->programObject, "a_position");
    p_state->user_data->texCoordLoc
        = glGetAttribLocation(p_state->user_data->programObject, "a_texCoord");

    // Get the sampler location
    p_state->user_data->samplerLoc
        = glGetUniformLocation(p_state->user_data->programObject, "s_texture");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    return TRUE;
}

void init_ogl(Target_State* state, int width, int height)
{
    EGLConfig config;
    EGLint num_config;
    EGLint major_version;
    EGLint minor_version;
    EGLBoolean result;

    // get an EGL display connection
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // initialize the EGL display connection
    result = eglInitialize(state->display, &major_version, &minor_version);
    assert(EGL_FALSE != result);

    // Get configs
    result = eglGetConfigs(state->display, NULL, 0, &num_config);
    assert(EGL_FALSE != result);

    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);

    // get an appropriate EGL frame buffer configuration
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);

    // create an EGL rendering context
    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
    assert(state->context != EGL_NO_CONTEXT);

    // create an EGL window surface
    state->xDisplay = XOpenDisplay(NULL);
    if (state->xDisplay == NULL) {
        printf("Sorry to say we can't create an Xwindow and this will fail");
        exit(0); // return; we need to trap this.
    }

    Window root;
    Window win;
    Screen* screen;
    XSetWindowAttributes swa;
    XSetWindowAttributes xattr;
    XWMHints hints;
    Atom wm_state;
    XEvent xev;

    root = DefaultRootWindow(state->xDisplay);
    screen = ScreenOfDisplay(state->xDisplay, 0);

    state->width = width;
    state->height = height;

    swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask | KeyReleaseMask;
    swa.background_pixmap = None;
    swa.background_pixel = 0;
    swa.border_pixel = 0;
    swa.override_redirect = TRUE;

    win = XCreateWindow(state->xDisplay, root, 0, 0, width, height, 0, CopyFromParent, InputOutput,
        CopyFromParent, CWEventMask, &swa);

    XSelectInput(state->xDisplay, win, KeyPressMask | KeyReleaseMask);

    xattr.override_redirect = TRUE;
    XChangeWindowAttributes(state->xDisplay, win, CWOverrideRedirect, &xattr);

    hints.input = TRUE;
    hints.flags = InputHint;
    XSetWMHints(state->xDisplay, win, &hints);

    char* title = (char*)"x11 window Triangle Example";
    // Make the window visible on the screen
    XMapWindow(state->xDisplay, win);
    XStoreName(state->xDisplay, win, title);

    // Get identifiers for the provided atom name strings
    wm_state = XInternAtom(state->xDisplay, "_NET_WM_STATE", FALSE);

    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = FALSE;
    XSendEvent(
        state->xDisplay, DefaultRootWindow(state->xDisplay), FALSE, SubstructureNotifyMask, &xev);

    state->nativeWindow = (EGLNativeWindowType)win;

    // get a surface surface
    state->surface = eglCreateWindowSurface(state->display, config, state->nativeWindow, NULL);
    assert(state->surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);
}

// void init_ogl_orig(Target_State* state, int width, int height)
//{
//    int32_t success = 0;
//    EGLBoolean result;
//    EGLint num_config;
//
//    // RPI setup is a little different to normal EGL
//    DISPMANX_ELEMENT_HANDLE_T DispmanElementH;
//    DISPMANX_DISPLAY_HANDLE_T DispmanDisplayH;
//    DISPMANX_UPDATE_HANDLE_T DispmanUpdateH;
//    VC_RECT_T dest_rect;
//    VC_RECT_T src_rect;
//    EGLConfig config;
//
//    // get an EGL display connection
//    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
//
//    // initialize the EGL display connection
//    result = eglInitialize(state->display, NULL, NULL);
//
//    // get an appropriate EGL frame buffer configuration
//    result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
//    assert(EGL_FALSE != result);
//
//    // get an appropriate EGL frame buffer configuration
//    result = eglBindAPI(EGL_OPENGL_ES_API);
//    assert(EGL_FALSE != result);
//
//    // create an EGL rendering context
//    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT,
//    context_attributes); assert(state->context != EGL_NO_CONTEXT);
//
//    // create an EGL window surface
//
//    state->width = width;
//    state->height = height;
//
//    dest_rect.x = 0;
//    dest_rect.y = 0;
//    dest_rect.width = state->width; // it needs to know our window size
//    dest_rect.height = state->height;
//
//    src_rect.x = 0;
//    src_rect.y = 0;
//
//    DispmanDisplayH = vc_dispmanx_display_open(0);
//    DispmanUpdateH = vc_dispmanx_update_start(0);
//
//    DispmanElementH = vc_dispmanx_element_add(DispmanUpdateH, DispmanDisplayH, 0 /*layer*/,
//        &dest_rect, 0 /*source*/, &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha value*/,
//        0 /*clamp*/, (DISPMANX_TRANSFORM_T)0 /*transform*/);
//
//    state->nativewindow.element = DispmanElementH;
//    state->nativewindow.width = state->width;
//    state->nativewindow.height = state->height;
//    vc_dispmanx_update_submit_sync(DispmanUpdateH);
//
//    state->surface = eglCreateWindowSurface(state->display, config, &(state->nativewindow),
//    NULL); assert(state->surface != EGL_NO_SURFACE);
//
//    // connect the context to the surface
//    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
//    assert(EGL_FALSE != result);
//}

/*****************************************
Draw a Rectangle with texture this is a hard coded
draw which is only good for the Rectangle
******************************************/
void Draw(Target_State* p_state)
{
    GLfloat RectVertices[] = {
        -0.5f, // data has to be position then normalised texture coord
        0.5f,
        0.0f, // Position 0
        0.0f,
        0.0f, // TexCoord 0
        -0.5f, -0.5f,
        0.0f, // Position 1
        0.0f,
        1.0f, // TexCoord 1
        0.5f, -0.5f,
        0.0f, // Position 2
        1.0f,
        1.0f, // TexCoord 2
        0.5f, 0.5f,
        0.0f, // Position 3
        1.0f,
        0.0f // TexCoord 3
    };

    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    // Setup the viewport
    glViewport(0, 0, p_state->width, p_state->height);
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);
    // setup the program object
    glUseProgram(p_state->user_data->programObject);
    // Load the vertex position
    glVertexAttribPointer(
        p_state->user_data->positionLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), RectVertices);
    // Load the texture coordinate
    glVertexAttribPointer(p_state->user_data->texCoordLoc, 2, GL_FLOAT, GL_FALSE,
        5 * sizeof(GLfloat), &RectVertices[3]);

    glEnableVertexAttribArray(p_state->user_data->positionLoc);
    glEnableVertexAttribArray(p_state->user_data->texCoordLoc);
    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, p_state->user_data->textureId);
    // actually draw the rect as 2 sets of 3 vertices (2 tris make a rect)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    if (glGetError() != GL_NO_ERROR)
        printf("Oh bugger");
}

void esInitContext(Target_State* p_state)
{
    if (p_state != NULL) {
        memset(p_state, 0, sizeof(Target_State));
    }
}

void esRegisterDrawFunc(Target_State* p_state, void (*draw_func)(Target_State*))
{
    p_state->draw_func = draw_func;
}

void esMainLoop(Target_State* esContext)
{
    int Counter = 0;
    while (Counter++ < 200) {
        if (esContext->draw_func != NULL)
            esContext->draw_func(esContext);
        // after our draw we need to swwap buffers to display
        eglSwapBuffers(esContext->display, esContext->surface);
    }
}

int main(int argc, char* argv[])
{
    UserData user_data;

    esInitContext(p_state);

    init_ogl(p_state, 1024, 768);
    p_state->user_data = &user_data;

    if (!Init(p_state))
        return 0;
    esRegisterDrawFunc(p_state, Draw);

    MyFiles FileHandler;

    // now go do the graphic loop
    int WhichImage = 0;
    int how_many_loops = 1;
    while (TRUE) {
        int Width, Height;
        char* OurRawData = FileHandler.Load(Images[WhichImage], &Width, &Height);

        if (OurRawData == NULL)
            printf("We failed to load\n");
        p_state->user_data->textureId = CreateTexture2D(Width, Height, OurRawData);

        esMainLoop(p_state);

        glDeleteTextures(1, &p_state->user_data->textureId);
        free(OurRawData);
        WhichImage++;
        if (WhichImage == NUMBEROFPICS)
            WhichImage = 0;
        printf("We've done %i loops \n", how_many_loops);
        how_many_loops++;
    }
}
