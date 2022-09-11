/*
 * Hello triangle
 */

#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

typedef struct
{
	GLuint program_object;
} UserData;

typedef struct TargetState
{
	uint32_t width;
	uint32_t height;

	Display *x_display;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLNativeWindowType native_window;

	UserData *user_data;
	void (*draw_func)(struct TargetState *);
} TargetState;

TargetState state;
TargetState *p_state = &state;

static const EGLint attribute_list[] = {EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
										EGL_ALPHA_SIZE, 8, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE};

GLuint load_shader(GLenum type, const char *shader_src)
{
	GLuint shader = glCreateShader(type);

	if (shader == 0)
	{
		return 0;
	}
	glShaderSource(shader, 1, &shader_src, NULL);
	glCompileShader(shader);

	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
		if (info_len > 1)
		{
			char *info_log = (char *)malloc(sizeof(char) * info_len);
			glGetShaderInfoLog(shader, info_len, NULL, info_log);
			fprintf(stderr, "Error compiling this shader:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

int init(TargetState *p_state)
{
	p_state->user_data = (UserData *)malloc(sizeof(UserData));

	GLbyte v_shader_str[] = "#version 300 es\n"
							"layout (location = 0) in vec4 v_Position;\n"
							"void main()\n"
							"{\n"
							"    gl_Position = v_Position;\n"
							"}\n";

	GLbyte f_shader_str[] = "#version 300 es\n"
							"precision mediump float;\n"
							"out vec4 frag_color;\n"
							"void main()\n"
							"{\n"
							"    frag_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
							"}\n";

	GLuint program_object, vertex_shader, fragment_shader;

	vertex_shader = load_shader(GL_VERTEX_SHADER, (char *)v_shader_str);
	fragment_shader = load_shader(GL_FRAGMENT_SHADER, (char *)f_shader_str);

	program_object = glCreateProgram();
	if (program_object == 0)
	{
		return 0;
	}

	glAttachShader(program_object, vertex_shader);
	glAttachShader(program_object, fragment_shader);

	glLinkProgram(program_object);

	GLint linked;
	glGetProgramiv(program_object, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLint info_len = 0;
		glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_len);
		if (info_len > 1)
		{
			char *info_log = (char *)malloc(sizeof(char) * info_len);
			glGetProgramInfoLog(program_object, info_len, NULL, info_log);
			fprintf(stderr, "Error linking program:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteProgram(program_object);
		return FALSE;
	}

	p_state->user_data->program_object = program_object;

	return TRUE;
}

void init_ogl(TargetState *state, int width, int height)
{
	state->width = width;
	state->height = height;

	EGLint num_configs;
	EGLint major_version;
	EGLint minor_version;

	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint context_atttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE};

	Window root;
	XSetWindowAttributes swa;
	XSetWindowAttributes xattr;
	Atom wm_state;
	XWMHints hints;
	XEvent xev;
	EGLConfig ecfg;
	EGLint num_config;
	Window win;
	Screen *screen;

	Display *x_display = state->x_display;
	x_display = XOpenDisplay(NULL);
	if (x_display == NULL)
	{
		printf("Sorry to say we can't create an Xwindow and this will fail");
		exit(0); // return; we need to trap this.
	}
	eglBindAPI(EGL_OPENGL_ES_API);
	root = DefaultRootWindow(x_display);
	screen = ScreenOfDisplay(x_display, 0);

	state->width = width;
	state->height = height;

	swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask | KeyReleaseMask;
	swa.background_pixmap = None;
	swa.background_pixel = 0;
	swa.border_pixel = 0;
	swa.override_redirect = TRUE;

	win = XCreateWindow(x_display, root, 0, 0, width, height, 0, CopyFromParent, InputOutput,
						CopyFromParent, CWEventMask, &swa);

	XSelectInput(x_display, win, KeyPressMask | KeyReleaseMask);

	xattr.override_redirect = TRUE;
	XChangeWindowAttributes(x_display, win, CWOverrideRedirect, &xattr);

	hints.input = TRUE;
	hints.flags = InputHint;
	XSetWMHints(x_display, win, &hints);

	char *title = (char *)"x11 window Triangle Example";
	// Make the window visible on the screen
	XMapWindow(x_display, win);
	XStoreName(x_display, win, title);

	// Get identifiers for the provided atom name strings
	wm_state = XInternAtom(x_display, "_NET_WM_STATE", FALSE);

	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = FALSE;
	XSendEvent(x_display, DefaultRootWindow(x_display), FALSE, SubstructureNotifyMask, &xev);

	state->native_window = (EGLNativeWindowType)win;

	// Get display
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY)
	{
		printf("Sorry to say we have an EGLinit error and this will fail");
		EGLint err = eglGetError();
		return; // EGL_FALSE;
	}

	// Initialise EGL
	if (!eglInitialize(display, &major_version, &minor_version))
	{
		printf("Sorry to say we have an EGLinit error and this will fail");
		EGLint err = eglGetError();
		return; // EGL_FALSE;
	}

	// Get configs
	if (!eglGetConfigs(display, NULL, 0, &num_configs))
	{
		printf("Sorry to say we have EGL config errors and this will fail");
		EGLint err = eglGetError();
		return; // EGL_FALSE;
	}

	// Choose config
	if (!eglChooseConfig(display, attribute_list, &config, 1, &num_configs))
	{
		printf("Sorry to say we have config choice issues and this will fail");
		EGLint err = eglGetError();
		return; // EGL_FALSE;
	}

	// Create a surface
	surface = eglCreateWindowSurface(display, config, state->native_window, NULL);
	if (surface == EGL_NO_SURFACE)
	{
		EGLint err = eglGetError();
		return; // EGL_FALSE;
	}

	// Create a GL context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_atttribs);
	if (context == EGL_NO_CONTEXT)
	{
		EGLint err = eglGetError();
		return; // EGL_FALSE;
	}

	// Make the context current
	if (!eglMakeCurrent(display, surface, surface, context))
	{
		EGLint err = eglGetError();
		return; // EGL_FALSE;
	}

	state->display = display;
	state->surface = surface;
	state->context = context;

	// just for fun lets see what we can do with this GPU
	printf("This SBC supports version %i.%i of EGL\n", major_version, minor_version);
	printf("This GPU supplied by  :%s\n", glGetString(GL_VENDOR));
	printf("This GPU supports     :%s\n", glGetString(GL_VERSION));
	printf("This GPU Renders with :%s\n", glGetString(GL_RENDERER));
	printf("This GPU supports     :%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("This GPU supports these extensions    :%s\n", glGetString(GL_EXTENSIONS));
	fflush(stdout);

	EGLBoolean gl_test = eglGetConfigAttrib(display, config, EGL_MAX_SWAP_INTERVAL, &minor_version);

	// 1 to lock speed to 60fps (assuming we are able to maintain it)
	// 0 for immediate swap (may cause tearing) which will indicate actual frame rate
	EGLBoolean test = eglSwapInterval(display, 1);

	if (glGetError() == GL_NO_ERROR)
	{
		return;
	}
	else
	{
		printf("Oh bugger, Some part of the EGL/OGL graphic init failed\n");
	}
}

void draw(TargetState *p_state)
{
	UserData *user_data = p_state->user_data;
	GLfloat triangle_vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};

	glViewport(0, 0, p_state->width, p_state->height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(user_data->program_object);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, triangle_vertices);
	glEnableVertexAttribArray(0);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	if (glGetError() != GL_NO_ERROR)
	{
		printf("Oh bugger");
	}
}

void es_init_context(TargetState *p_state)
{
	if (p_state != NULL)
	{
		memset(p_state, 0, sizeof(TargetState));
	}
}

void es_register_draw_func(TargetState *p_state, void (*draw_func)(TargetState *))
{
	p_state->draw_func = draw_func;
}

void es_main_loop(TargetState *es_context)
{
	int counter = 0;
	while (counter++ < 200)
	{
		if (es_context->draw_func != NULL)
		{
			es_context->draw_func(es_context);
		}
		eglSwapBuffers(es_context->display, es_context->surface);
	}
}

int main(int argc, char *argv[])
{
	UserData user_data;
	es_init_context(p_state);
	init_ogl(p_state, 1024, 720);
	p_state->user_data = &user_data;

	if (!init(p_state))
	{
		return 0;
	}
	es_register_draw_func(p_state, draw);

	es_main_loop(p_state);
}
