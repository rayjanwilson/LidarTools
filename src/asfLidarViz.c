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
#include <math.h>       // Header file for trigonometric functions.

/* ascii codes for various special keys */
#define ESCAPE 27
#define PAGE_UP 73
#define PAGE_DOWN 81
#define UP_ARROW 72
#define DOWN_ARROW 80
#define LEFT_ARROW 75
#define RIGHT_ARROW 77

/* The number of our GLUT window */
int window;
GLuint loop;             // general loop variable
int fullScreen = 0;

int light = 0;           // lighting on/off
int blend = 0;        // blending on/off

GLfloat xrot;            // x rotation
GLfloat yrot;            // y rotation
GLfloat xspeed;          // x rotation speed
GLfloat yspeed;          // y rotation speed

GLfloat lookupdown = 0.0;
const float piover180 = 0.0174532925f;

float heading, xpos, zpos;

GLfloat camx = 0, camy = 0, camz = 0; // camera location.
GLfloat therotate;

GLfloat z=0.0f;                       // depth into the screen.

GLfloat LightAmbient[]  = {0.5f, 0.5f, 0.5f, 1.0f};
GLfloat LightDiffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat LightPosition[] = {0.0f, 0.0f, 2.0f, 1.0f};

GLuint filter = 0;       // texture filtering method to use (nearest, linear, linear + mipmaps)

// degrees to radians...2 PI radians = 360 degrees
float rad(float angle)
{
    return angle * piover180;
}

typedef struct {         // vertex coordinates - 3d and texture
    GLfloat x, y, z;     // 3d coords.
} VERTEX;

typedef struct {         // sector of a 3d environment
    int numpoints;    // number of triangles in the sector
    float maxheight;
    float minheight;
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
    float maxheight, minheight;
    FILE *filein;        // file to load the world from
    char oneline[255];

    filein = fopen("Data/test.txt", "rt");

    readstr(filein, oneline);
    sscanf(oneline, "NUMPOINTS %i\n", &numpoints);

    readstr(filein, oneline);
    sscanf(oneline, "MAXHEIGHT %f\n", &maxheight);

    readstr(filein, oneline);
    sscanf(oneline, "MINHEIGHT %f\n", &minheight);


    sector1.numpoints = numpoints;
    sector1.maxheight = maxheight;
    sector1.minheight = minheight;
    sector1.point = (VERTEX *) malloc(sizeof(VERTEX)*numpoints);

    printf("number of points %i\n", numpoints);
    printf("max height %f\n", maxheight);
    printf("min height %f\n", minheight);
    for (loop = 0; loop < numpoints; loop++) {

        readstr(filein,oneline);
        sscanf(oneline, "%f %f %f", &x, &y, &z);
        printf("Loaded %f %f %f\n", x, y, z);
        sector1.point[loop].x = x;
        sector1.point[loop].y = y;
        sector1.point[loop].z = z;
        float fColor = (z)/sector1.maxheight;
        printf("  fcolor: %f\n", fColor);
        printf("  rgb color: %f %f %f\n", 1-fColor, fColor/2, fColor);
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

    // set up lights.
    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
    glEnable(GL_LIGHT1);
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

  // set up lights.
    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
    glEnable(GL_LIGHT1);

}



/* The main drawing function. */
void DrawGLScene()
{
    GLfloat xm, ym, zm;
    GLfloat xtrans, ztrans, ytrans;
    GLfloat sceneroty;
    int numpoints;

    // calculate translations and rotations.
    xtrans = -xpos;
    ztrans = -zpos;
    ytrans = 0;

    sceneroty = 360.0f - yrot;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // Clear The Screen And The Depth Buffer
    glLoadIdentity();             // Reset The View

    if (fullScreen)
        glutFullScreen();
    else
        glutReshapeWindow(640, 480);

    glRotatef(lookupdown, 1.0f, 0, 0);
    glRotatef(sceneroty, 0, 1.0f, 0);

    glTranslatef(xtrans, ytrans, ztrans);

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
            float fColor = (zm)/sector1.maxheight;
            glColor3f(fColor, fColor/2, 1-fColor);
            glVertex3f( xm, ym, zm);
        glEnd();
    }

    // swap buffers to display, since we're double buffered.
    glutSwapBuffers();
}

/* The function called whenever a normal key is pressed. */
void keyPressed(unsigned char key, int x, int y)
{
    /* avoid thrashing this procedure */
    usleep(100);

    switch (key) {
    case ESCAPE: // kill everything.
    /* exit the program...normal termination. */
    exit(1);
    break; // redundant.

    case 'b':
    case 'B': // switch the blending
    printf("B/b pressed; blending is: %d\n", blend);
    blend = blend ? 0 : 1;              // switch the current value of blend, between 0 and 1.
    if (blend) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
    printf("Blending is now: %d\n", blend);
    break;

    case 'f':
    case 'F': // switch the filter
    printf("F/f pressed; filter is: %d\n", filter);
    filter++;                           // switch the current value of filter, between 0/1/2;
    if (filter > 2) {
        filter = 0;
    }
    printf("Filter is now: %d\n", filter);
    break;

    case 'l':
    case 'L': // switch the lighting
    printf("L/l pressed; lighting is: %d\n", light);
    light = light ? 0 : 1;              // switch the current value of light, between 0 and 1.
    if (light) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
    printf("Lighting is now: %d\n", light);
    break;

    default:
      printf ("Key %d pressed. No action there yet.\n", key);
      break;
    }
}

/* The function called whenever a normal key is pressed. */
void specialKeyPressed(int key, int x, int y)
{
    /* avoid thrashing this procedure */
    usleep(100);

    switch (key) {
    case GLUT_KEY_PAGE_UP: // tilt up
        z -= 0.2f;
        lookupdown -= 0.2f;
        break;

    case GLUT_KEY_PAGE_DOWN: // tilt down
        z += 0.2f;
        lookupdown += 1.0f;
        break;

    case GLUT_KEY_UP: // walk forward (bob head)
        xpos -= (float)sin(yrot*piover180) * 0.2f;
        zpos -= (float)cos(yrot*piover180) * 0.2f;
        break;

    case GLUT_KEY_DOWN: // walk back (bob head)
        xpos += (float)sin(yrot*piover180) * 0.2f;
        zpos += (float)cos(yrot*piover180) * 0.2f;
        break;

    case GLUT_KEY_LEFT: // look left
        yrot += 1.5f;
        break;

    case GLUT_KEY_RIGHT: // look right
        yrot -= 1.5f;
        break;

    default:
        printf ("Special key %d pressed. No action there yet.\n", key);
        break;
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

    /* Register the function called when special keys (arrows, page down, etc) are pressed. */
    glutSpecialFunc(&specialKeyPressed);

    /* Initialize our window. */
    InitGL(640, 480);

    /* Start Event Processing Engine */
    glutMainLoop();

    return 1;
}
