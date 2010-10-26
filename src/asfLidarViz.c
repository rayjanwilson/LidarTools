//
// This code was created by Jeff Molofee '99 (ported to Linux/GLUT by Richard Campbell '99)
//
// If you've found this code useful, please let me know.
//
// Visit me at www.demonews.com/hosted/nehe
// (email Richard Campbell at ulmont@bellsouth.net)
//
#include <GL/glut.h>    // Header File For The GLUT Library
#include <GL/gl.h>  // Header File For The OpenGL32 Library
#include <GL/glu.h> // Header File For The GLu32 Library
#include <unistd.h>     // Header File For sleeping.
#include <stdio.h>      // Header file for standard file i/o.
#include <stdlib.h>     // Header file for malloc/free.

/* ASCII code for the escape key. */
#define ESCAPE 27

/* The number of our GLUT window */
int window;
GLuint loop;             // general loop variable
int fullScreen = 0;

typedef struct {         // vertex coordinates - 3d and texture
    GLfloat x, y, z;     // 3d coords.
    //GLfloat u, v;        // texture coords.
} VERTEX;

typedef struct {         // sector of a 3d environment
    int numpoints;    // number of triangles in the sector
    VERTEX* point;  // pointer to array of points.
} SECTOR;

SECTOR sector1;

// helper for SetupWorld.  reads a file into a string until a nonblank, non-comment line
// is found ("/" at the start indicating a comment); assumes lines < 255 characters long.
void readstr(FILE *f, char *string)
{
    do {
        fgets(string, 255, f); // read the line
    } while ((string[0] == '/') || (string[0] == '\n'));
    return;
}

// loads the world from a text file.
void SetupWorld()
{
    float x, y, z;
    int numpoints;
    FILE *filein;        // file to load the world from
    char oneline[255];

    filein = fopen("Data/test.txt", "rt");

    readstr(filein, oneline);
    sscanf(oneline, "NUMPOINTS %d\n", &numpoints);
    printf("number of points %i\n", numpoints);
    sector1.numpoints = numpoints;
    sector1.point = (VERTEX *) malloc(sizeof(VERTEX)*numpoints);

    for (loop = 0; loop < numpoints; loop++) {

        readstr(filein,oneline);
        sscanf(oneline, "%f %f %f", &x, &y, &z);
        printf("Loaded %f %f %f\n", x, y, z);
        sector1.point[loop].x = x;
        sector1.point[loop].y = y;
        sector1.point[loop].z = z;
    }

    fclose(filein);
    return;
}

/* A general OpenGL initialization function.  Sets all of the initial parameters. */
void InitGL(int Width, int Height)          // We call this right after our OpenGL window is created.
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);     // This Will Clear The Background Color To Black
    glClearDepth(1.0);                // Enables Clearing Of The Depth Buffer
    glDepthFunc(GL_LESS);             // The Type Of Depth Test To Do
    glEnable(GL_DEPTH_TEST);          // Enables Depth Testing
    glShadeModel(GL_SMOOTH);          // Enables Smooth Color Shading
    glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);                                             // Set Perspective Calculations To Most Accurate
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();             // Reset The Projection Matrix

    gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f); // Calculate The Aspect Ratio Of The Window

    glMatrixMode(GL_MODELVIEW);
}

/* The function called when our window is resized (which shouldn't happen, because we're fullscreen) */
void ReSizeGLScene(int Width, int Height)
{
  if (Height==0)                // Prevent A Divide By Zero If The Window Is Too Small
    Height=1;

  glViewport(0, 0, Width, Height);      // Reset The Current Viewport And Perspective Transformation

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);
  glMatrixMode(GL_MODELVIEW);
}



/* The main drawing function. */
void DrawGLScene()
{
    GLfloat xm, ym, zm;
    int numpoints;

    if (fullScreen)
        glutFullScreen();
    else
        glutReshapeWindow(640, 480);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // Clear The Screen And The Depth Buffer
    glLoadIdentity();             // Reset The View

    //glTranslatef(0.0f,0.0f,-4.0f);//move forward 4 units

    // Position Camera 40 Meters Up In Z-Direction.
    // Set The Up Vector In Y-Direction So That +X Directs To Right And +Y Directs To Up On The Window.
    gluLookAt(0, 0, 40, 0, 0, 0, 0, 1, 0);

    numpoints = sector1.numpoints;

    for (loop=0; loop<numpoints; loop++) {        // loop through all the triangles
        glPointSize(4);
        glBegin(GL_POINTS);
            xm = sector1.point[loop].x;
            ym = sector1.point[loop].y;
            zm = sector1.point[loop].z;
            glVertex3f( xm, ym, zm);
        glEnd();
    }

    // swap buffers to display, since we're double buffered.
    glutSwapBuffers();
}

/* The function called whenever a key is pressed. */
void keyPressed(unsigned char key, int x, int y)
{
    /* avoid thrashing this procedure */
    usleep(100);

    /* If escape is pressed, kill everything. */
    if (key == ESCAPE)
    {
    /* shut down our window */
    glutDestroyWindow(window);

    /* exit the program...normal termination. */
    exit(0);
    }
}

int main(int argc, char **argv)
{
    /* load our world from disk */
    SetupWorld();

  /* Initialize GLUT state - glut will take any command line arguments that pertain to it or
     X Windows - look at its documentation at http://reality.sgi.com/mjk/spec3/spec3.html */
  glutInit(&argc, argv);

  /* Select type of Display mode:
     Double buffer
     RGBA color
     Alpha components supported
     Depth buffer */
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);

  /* get a 640 x 480 window */
  glutInitWindowSize(640, 480);

  /* the window starts at the upper left corner of the screen */
  glutInitWindowPosition(0, 0);

  /* Open a window */
  window = glutCreateWindow("ASF LidarViz");

  /* Register the function to do all our OpenGL drawing. */
  glutDisplayFunc(&DrawGLScene);

  /* Go fullscreen.  This is the soonest we could possibly go fullscreen. */
  //glutFullScreen();

  /* Even if there are no events, redraw our gl scene. */
  glutIdleFunc(&DrawGLScene);

  /* Register the function called when our window is resized. */
  glutReshapeFunc(&ReSizeGLScene);

  /* Register the function called when the keyboard is pressed. */
  glutKeyboardFunc(&keyPressed);

  /* Register the function called when the keyboard is pressed. */
  glutSpecialFunc(&keyPressed);

  /* Initialize our window. */
  InitGL(640, 480);

  /* Start Event Processing Engine */
  glutMainLoop();

  return 1;
}
