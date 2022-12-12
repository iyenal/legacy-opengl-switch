/*---------------------------------------------------------------------------------

	Gen7 Engine EGL + libnx + glad component 

---------------------------------------------------------------------------------*/

//{ Includes - General

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>

// Switch specific includes

#include <switch.h>
#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include "glad.h"
//#include <GL/glu.h> // EGL extensions
//#include "glu_engine.c"

static EGLDisplay s_display;
static EGLContext s_context;
static EGLSurface s_surface;

static bool initEgl(void){
	NWindow* win = nwindowGetDefault();
    nwindowSetDimensions(win, 1280, 720);
	
    // Connect to the EGL default display
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(s_display, NULL, NULL); 
    eglBindAPI(EGL_OPENGL_API);

    // Get an appropriate EGL framebuffer configuration
    EGLConfig config; EGLint numConfigs;
    static const EGLint framebufferAttributeList[] =
    {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_RED_SIZE,     8, EGL_GREEN_SIZE,   8,EGL_BLUE_SIZE,    8,
		EGL_ALPHA_SIZE,   8, EGL_DEPTH_SIZE,   24, EGL_STENCIL_SIZE, 8, EGL_NONE
	};
    eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);

    // Create an EGL window surface
    s_surface = eglCreateWindowSurface(s_display, config, nwindowGetDefault(), NULL);
    static const EGLint contextAttributeList[] =
    {
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
        EGL_CONTEXT_MAJOR_VERSION_KHR, 2, EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE
    };
    s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);

    // Connect the context to the surface, glad
    eglMakeCurrent(s_display, s_surface, s_surface, s_context);
	gladLoadGLLoader((GLADloadproc)eglGetProcAddress);

    return true;
}


// SDL/Switch: replacements for GLU
#if defined (Use_SDL_Window) || defined (__SWITCH__)

#define __glPi 3.14159265358979323846

static void __gluMakeIdentityd(GLdouble m[16]){
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

static void __gluMakeIdentityf(GLfloat m[16]){
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar){
    GLdouble m[4][4];
    double sine, cotangent, deltaZ;
    double radians = fovy / 2 * __glPi / 180;

    deltaZ = zFar - zNear;
    sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
	return;
    }
    cotangent = cos(radians) / sine;

    __gluMakeIdentityd(&m[0][0]);
    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1;
    m[3][2] = -2 * zNear * zFar / deltaZ;
    m[3][3] = 0;
    glMultMatrixd(&m[0][0]);
}

static void normalize(float v[3]){
    float r;

    r = sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
    if (r == 0.0) return;

    v[0] /= r;
    v[1] /= r;
    v[2] /= r;
}

static void cross(float v1[3], float v2[3], float result[3]){
    result[0] = v1[1]*v2[2] - v1[2]*v2[1];
    result[1] = v1[2]*v2[0] - v1[0]*v2[2];
    result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

void gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy,GLdouble upz){
    float forward[3], side[3], up[3];
    GLfloat m[4][4];

    forward[0] = centerx - eyex;
    forward[1] = centery - eyey;
    forward[2] = centerz - eyez;

    up[0] = upx;
    up[1] = upy;
    up[2] = upz;

    normalize(forward);

    /* Side = forward x up */
    cross(forward, up, side);
    normalize(side);

    /* Recompute up as: up = side x forward */
    cross(side, forward, up);

    __gluMakeIdentityf(&m[0][0]);
    m[0][0] = side[0];
    m[1][0] = side[1];
    m[2][0] = side[2];

    m[0][1] = up[0];
    m[1][1] = up[1];
    m[2][1] = up[2];

    m[0][2] = -forward[0];
    m[1][2] = -forward[1];
    m[2][2] = -forward[2];

    glMultMatrixf(&m[0][0]);
    glTranslated(-eyex, -eyey, -eyez);
}


#endif


// Engine OpenGL Entry
int InitGLScene(){

	//{char buffer[8192];sprintf(buffer, "Scene initialisation");MessageBox (NULL, buffer, "Debug Message", MB_OK);}
	
	#ifdef Use_SDL_Window 
	#elif defined(__SWITCH__)
	#else
	// initialize the window size ( for glut )
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	#endif

	
    glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
    glClearColor(0.0f, 0.5f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);								// Depth Buffer Setup


	GLfloat LightAmbient[]=		{ 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat LightDiffuse[]=		{ 2.0f, 2.0f, 2.0f, 2.0f };
	GLfloat LightPosition[]=	{ 3.0, 3.0, 5.0, 0.0 }; 
	GLfloat ambi[]= { 0.0f, 0.0f, 0.0f, 1.0f };				 // ambient Light Values ( NEW )
	GLfloat white[]= { 0.8f, 0.8f, 0.8f, 0.8f };				 // Diffuse Light Values ( NEW )
	GLfloat emis[]= { 0.0f, 0.0f, 0.0f, 1.0f };				 // emisive Light Values ( NEW )
	GLfloat spec[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat shin=   50.0f;
	GLfloat globambi[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat no_mat[]= { 0.0f, 0.0f, 0.0f, 1.0f };
	//direction = [0.0f, 0.0f, -1.0f, 1.0];


	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT0, GL_POSITION,LightPosition);	// Position The Light

	// set up material
	glMaterialfv(GL_FRONT, GL_AMBIENT , ambi);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT, GL_EMISSION, emis); //was emis
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); //specular material
	glMaterialf(GL_FRONT, GL_SHININESS, shin); //shininess

	//set up global ambient light
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globambi);

	glEnable(GL_LIGHT0); 
	glEnable(GL_LIGHTING);		// Enable Lighting
	
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	
	glEnable(GL_ALPHA);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//{char buffer[8192];sprintf(buffer, "Engine Kernel well loaded");MessageBox (NULL, buffer, "Evo7 Debug Message", MB_OK);}
	
	// Centerize object.
	glTranslatef(-1.0f,-1.0f,-1.0f);

	return TRUE;										// Initialization Went OK
}

// this is where you draw your opengl scene (called each loop by Display)
int DrawGLScene() {
	
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	
	// Last argument: depth length (far clip)
	gluPerspective(65.0f, (GLfloat) SCREEN_WIDTH/(GLfloat) SCREEN_HEIGHT, 1.0f, (GLfloat) DEPTH_MAX);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	
	//glClearDepth(1);
	glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glPushMatrix();
	
	glClearDepth(1.0f);									// Depth Buffer Setup
	glDisable(GL_LIGHT0); 
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	//glEnable(GL_ALPHA);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	
	if(varRenderSettings[0] == 1){
		glEnable(GL_LIGHT0); 
		glEnable(GL_LIGHTING); // Lighting mode
	}
	if(varRenderSettings[1] == 1)
		glEnable(GL_TEXTURE_2D); // Textured mode
	if(varRenderSettings[2] == 1)
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); // Wireframe mode
	
	//renderSkybox();

	// Setting camera
	
	// Blender Setup
	glRotatef(varCameraLoc[3], 1.0f, 0.0f, 0.0f);
	glRotatef(varCameraLoc[5], 0.0f, 1.0f, 0.0f);
	glRotatef(-varCameraLoc[4], 0.0f, 0.0f, 1.0f);
	glTranslatef(-varCameraLoc[0], -varCameraLoc[2], varCameraLoc[1]);
	//glScalef(sX,sY,sZ);

	// Rendering scene
	renderAnimations();
	renderSceneModels();
	renderSceneAll();
	
	glPopMatrix();
	glLoadIdentity();
	renderSceneSprites();

	return TRUE;	// Keep Going
	
}

int main(int argc, char **argv ){

    // Initialize EGL on the default window
    if (!initEgl())
        return EXIT_FAILURE;
	
	//Initialize OpenGL (custom)
	InitGLScene();
	
	fclose(stdout);
	stdout = saved;

    // Main graphics loop
    while (appletMainLoop())
	{
		DrawGLScene();
		glFlush();
        eglSwapBuffers(s_display, s_surface);
    }

	return 0;
}

