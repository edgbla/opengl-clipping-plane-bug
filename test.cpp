#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>

//-----------------------------------------------------------------------------

// GL_ARB_shader_objects
PFNGLCREATEPROGRAMOBJECTARBPROC  glCreateProgramObjectARB  = NULL;
PFNGLDELETEOBJECTARBPROC         glDeleteObjectARB         = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC     glUseProgramObjectARB     = NULL;
PFNGLCREATESHADEROBJECTARBPROC   glCreateShaderObjectARB   = NULL;
PFNGLSHADERSOURCEARBPROC         glShaderSourceARB         = NULL;
PFNGLCOMPILESHADERARBPROC        glCompileShaderARB        = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB = NULL;
PFNGLATTACHOBJECTARBPROC         glAttachObjectARB         = NULL;
PFNGLGETINFOLOGARBPROC           glGetInfoLogARB           = NULL;
PFNGLLINKPROGRAMARBPROC          glLinkProgramARB          = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC   glGetUniformLocationARB   = NULL;
PFNGLUNIFORM4FARBPROC            glUniform4fARB            = NULL;
PFNGLUNIFORM1IARBPROC            glUniform1iARB            = NULL;
PFNGLUNIFORM2FARBPROC            glUniform2fARB            = NULL;

//-----------------------------------------------------------------------------

static Display *dpy;
static Window win;

static GLuint datatexID;

static GLhandleARB g_programObj;
static GLhandleARB g_vertexShader;
static GLhandleARB g_fragmentShader;

static int enabled = 1;
static int brightness = 8;

//-----------------------------------------------------------------------------

void redraw()
{
	// Clear the screen and the depth buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    // Set matrices
    glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho(0, 640, 0, 480, -1, 1);
	glViewport(0, 0, 640, 480);
	glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
	
	// STAGE 0
	glClientActiveTextureARB( GL_TEXTURE0_ARB );
	glActiveTextureARB( GL_TEXTURE0_ARB );
	glBindTexture( GL_TEXTURE_2D, datatexID );
	glEnable( GL_TEXTURE_2D );
	
	glColor3f( 1.0, 1.0, 1.0 );
	
	GLdouble plane[4] = {1.0, 0.0, 0.0, 0.0};
	
	glClipPlane (GL_CLIP_PLANE0, plane);
    glEnable (GL_CLIP_PLANE0);
	
	if(enabled)
	{
		glUseProgramObjectARB( g_programObj );
		
		GLint texture = glGetUniformLocationARB( g_programObj, "OGL2Texture" );
		GLfloat param = glGetUniformLocationARB( g_programObj, "OGL2Param" );
		glUniform1iARB( texture, 0 );
		glUniform4fARB( param, 0, 0, brightness, 0 );
	}
	
	// Draw
	glBegin( GL_QUADS );
		glTexCoord2f( 0.0f, 0.0f );
		glVertex3f( 0.0f, 0.0f, 0.0f );
		glTexCoord2f( 1.0f, 0.0f );
		glVertex3f( 640.0f, 0.0f, 0.0f );
		glTexCoord2f( 1.0f, 1.0f );
		glVertex3f( 640.0f, 480.0f, 0.0f );
		glTexCoord2f( 0.0f, 1.0f );
		glVertex3f( 0.0f, 480.0f, 0.0f );
	glEnd();
	
	if(enabled)
	{
		glUseProgramObjectARB( NULL );
	}
	
	glDisable (GL_CLIP_PLANE0);
	
	glXSwapBuffers( dpy, win );
}

//-----------------------------------------------------------------------------

void event_loop()
{
	bool running = true;
	XEvent event;
	while( running )
	{
		while( XPending(dpy) > 0 )
		{
			XNextEvent( dpy, &event );
			switch( event.type )
			{
				case KeyPress:
           	 	case KeyRelease:
				switch( XLookupKeysym(&event.xkey,0) )
				{
                	case XK_Left:  break;
                	case XK_Right: break;
                	case XK_Up:    if( brightness < 17 ) brightness++; break;
                	case XK_Down:  if( brightness >  0 ) brightness--; break;
                	case XK_space: enabled ^= 1; break;
                	case XK_Escape: running = false; break;
				}
				break;
			}
		}
		redraw();
		usleep( 200 );
	}
}

//-----------------------------------------------------------------------------

unsigned char* LoadBMP( char *BitmapName, int &width, int &height )
{
    FILE *file = NULL;
    unsigned short nNumPlanes;
    unsigned short nNumBPP;
    unsigned char *data;
    int i;
    if( (file = fopen(BitmapName, "rb") ) == NULL )
       fprintf(stderr, "ERROR: getBitmapImageData - %s not found.\n",BitmapName);
    // Seek forward to width and height info
    fseek( file, 18, SEEK_CUR );
    if( (i = fread(&width, 4, 1, file) ) != 1 )
        fprintf(stderr, "ERROR: getBitmapImageData - Couldn't read width from %s.\n", BitmapName);
    if( (i = fread(&height, 4, 1, file) ) != 1 )
        fprintf(stderr, "ERROR: getBitmapImageData - Couldn't read height from %s.\n", BitmapName);
    if( (fread(&nNumPlanes, 2, 1, file) ) != 1 )
        fprintf(stderr, "ERROR: getBitmapImageData - Couldn't read plane count from %s.\n", BitmapName);
    if( nNumPlanes != 1 )
        fprintf(stderr, "ERROR: getBitmapImageData - Plane count from %s is not 1: %u\n", BitmapName, nNumPlanes);
    if( (i = fread(&nNumBPP, 2, 1, file)) != 1 )
       fprintf(stderr, "ERROR: getBitmapImageData - Couldn't read BPP from %s.\n", BitmapName);
    if( nNumBPP != 24 )
        fprintf(stderr, "ERROR: getBitmapImageData - BPP from %s is not 24: %u\n", BitmapName, nNumBPP);
    // Seek forward to image data
    fseek( file, 24, SEEK_CUR );
    // Calculate the image's total size in bytes. Note how we multiply the 
    // result of (width * height) by 3. This is becuase a 24 bit color BMP 
    // file will give you 3 bytes per pixel.
    int nTotalImagesize = width * height * 3;
    data = new unsigned char[nTotalImagesize];
    if( (i = fread(data, nTotalImagesize, 1, file) ) != 1 )
        fprintf(stderr, "ERROR: getBitmapImageData - Couldn't read image data from %s.\n", BitmapName);
    // Finally, rearrange BGR to RGB
    char charTemp;
    for( i = 0; i < nTotalImagesize; i += 3 ) {
        charTemp = data[i];
        data[i] = data[i+2];
        data[i+2] = charTemp;
    }
    fclose( file );
	
    return data;
}

//-----------------------------------------------------------------------------

void loadTextures( void )
{
	unsigned char *data;
	int width, height;
	
    data = LoadBMP( "./test.bmp", width, height );
	glGenTextures( 1, &datatexID );
	glBindTexture( GL_TEXTURE_2D, datatexID );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data ); 
	delete [] data;
}

//-----------------------------------------------------------------------------

unsigned char *readShaderFile( const char *fileName )
{
	FILE *file = fopen( fileName, "r" );
	if( file == NULL )
	{
		printf( "Cannot open shader file! %s \n", fileName );
		return 0;
	}
	
	unsigned char *buffer = new unsigned char[0xffff];
	int bytes = fread( buffer, 1, 0xffff, file );
	buffer[bytes] = 0;
	
	fclose( file );
	
	return buffer;
}

//-----------------------------------------------------------------------------

void initShader( void )
{
	// If the required extension is present, get the addresses of its 
	// functions that we wish to use...
	char *ext = (char*)glGetString( GL_EXTENSIONS );
	
	if( strstr( ext, "GL_ARB_shading_language_100" ) == NULL )
	{
		// This extension string indicates that the OpenGL Shading Language,
		// version 1.00, is supported.
		printf( "GL_ARB_shading_language_100 extension was not found\n" );
		return;
	}
	
	if( strstr( ext, "GL_ARB_shader_objects" ) == NULL )
	{
		printf( "GL_ARB_shader_objects extension was not found\n" );
		return;
	}
	else
	{
        glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)glXGetProcAddressARB((GLubyte*)"glCreateProgramObjectARB");
        glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC)glXGetProcAddressARB((GLubyte*)"glDeleteObjectARB");
        glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC)glXGetProcAddressARB((GLubyte*)"glUseProgramObjectARB");
        glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)glXGetProcAddressARB((GLubyte*)"glCreateShaderObjectARB");
        glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC)glXGetProcAddressARB((GLubyte*)"glShaderSourceARB");
        glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC)glXGetProcAddressARB((GLubyte*)"glCompileShaderARB");
        glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)glXGetProcAddressARB((GLubyte*)"glGetObjectParameterivARB");
        glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC)glXGetProcAddressARB((GLubyte*)"glAttachObjectARB");
        glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC)glXGetProcAddressARB((GLubyte*)"glGetInfoLogARB");
        glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC)glXGetProcAddressARB((GLubyte*)"glLinkProgramARB");
        glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)glXGetProcAddressARB((GLubyte*)"glGetUniformLocationARB");
        glUniform4fARB            = (PFNGLUNIFORM4FARBPROC)glXGetProcAddressARB((GLubyte*)"glUniform4fARB");
		glUniform1iARB            = (PFNGLUNIFORM1IARBPROC)glXGetProcAddressARB((GLubyte*)"glUniform1iARB");
		glUniform2fARB            = (PFNGLUNIFORM2FARBPROC)glXGetProcAddressARB((GLubyte*)"glUniform2fARB");

        if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
            !glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB || 
            !glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB || 
            !glLinkProgramARB || !glGetUniformLocationARB || !glUniform4fARB ||
			!glUniform1iARB || !glUniform2fARB )
        {
			printf( "One or more GL_ARB_shader_objects functions were not found\n" );
			return;
		}
	}
	
    const char *vertexShaderStrings[1];
    const char *fragmentShaderStrings[1];
    GLint bVertCompiled;
    GLint bFragCompiled;
    GLint bLinked;
    char str[4096];
	
    // Create the vertex shader...
    g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
	
	unsigned char *vertexShaderAssembly = readShaderFile( "./brightness.slv" );
    vertexShaderStrings[0] = (char*)vertexShaderAssembly;
    glShaderSourceARB( g_vertexShader, 1, vertexShaderStrings, NULL );
    glCompileShaderARB( g_vertexShader);
    delete [] vertexShaderAssembly;
	
    glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &bVertCompiled );
    if( bVertCompiled  == false )
	{
		glGetInfoLogARB(g_vertexShader, sizeof(str), NULL, str);
        sprintf( str, "Vertex Shader Compile Error" );
    }
	
	// Create the fragment shader...
    g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
	
    unsigned char *fragmentShaderAssembly = readShaderFile( "./brightness.slf" );
    fragmentShaderStrings[0] = (char*)fragmentShaderAssembly;
    glShaderSourceARB( g_fragmentShader, 1, fragmentShaderStrings, NULL );
    glCompileShaderARB( g_fragmentShader );
    delete [] fragmentShaderAssembly;
	
    glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &bFragCompiled );
    if( bFragCompiled == false )
	{
		glGetInfoLogARB( g_fragmentShader, sizeof(str), NULL, str );
		sprintf( str, "Fragment Shader Compile Error" );
	}
	
    // Create a program object and attach the two compiled shaders...
    g_programObj = glCreateProgramObjectARB();
    glAttachObjectARB( g_programObj, g_vertexShader );
    glAttachObjectARB( g_programObj, g_fragmentShader );
	
    // Link the program object and print out the info log...
    glLinkProgramARB( g_programObj );
    glGetObjectParameterivARB( g_programObj, GL_OBJECT_LINK_STATUS_ARB, &bLinked );
	
    if( bLinked == false )
	{
		glGetInfoLogARB( g_programObj, sizeof(str), NULL, str );
		printf( "Linking Error\n" );
	}
}

//-----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
	XVisualInfo			 *xvVisualInfo;
	Colormap			  cmColorMap;
	XSetWindowAttributes  winAttr;
	GLXContext  		  glXContext;
	int Attr[] = { GLX_RGBA,
				   GLX_RED_SIZE, 1,
				   GLX_GREEN_SIZE, 1,
				   GLX_BLUE_SIZE, 1,
				   GLX_DEPTH_SIZE, 16,
				   GLX_DOUBLEBUFFER,
				   None };
	
	// Connect to X Server
	dpy = XOpenDisplay(NULL);
	if(dpy == NULL) return 0; // Could not open display
	// GLX extension is supported?
	if(!glXQueryExtension(dpy, NULL, NULL)) return 0; // Return, if X server haven't GLX
	// Find visual
	xvVisualInfo = glXChooseVisual(dpy, DefaultScreen(dpy), Attr);
	if(xvVisualInfo == NULL) return 0; // If Visual info can't be shoosed
	// Create new colormap for our window
	cmColorMap = XCreateColormap(dpy, RootWindow(dpy, xvVisualInfo->screen), xvVisualInfo->visual, AllocNone);
	winAttr.colormap = cmColorMap;
	winAttr.border_pixel = 0;
	winAttr.background_pixel = 0;
	winAttr.event_mask = ExposureMask | ButtonPressMask | StructureNotifyMask | KeyPressMask;
	
	// Create window
	win = XCreateWindow(dpy, RootWindow(dpy, xvVisualInfo->screen),
						0, 0, 640, 480, 0,
						xvVisualInfo->depth,
						InputOutput,
						xvVisualInfo->visual,
						CWBorderPixel | CWColormap | CWEventMask,
						&winAttr);
	
	// Create OpenGL rendering context
	glXContext = glXCreateContext(dpy, xvVisualInfo, None, True);
	if(glXContext == NULL) return 0; // Can't create rendering context
	// Make it current	
	glXMakeCurrent(dpy, win, glXContext);
	// Map window on the display
	XMapWindow(dpy, win);
	
	initShader();
	loadTextures();
	
	// Enter to the main X loop
	event_loop();
	
	glDeleteTextures( 1, &datatexID );
	
	glDeleteObjectARB( g_vertexShader );
	glDeleteObjectARB( g_fragmentShader );
	glDeleteObjectARB( g_programObj );
	
	return 0;
}

//-----------------------------------------------------------------------------
