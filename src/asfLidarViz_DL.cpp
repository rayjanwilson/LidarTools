///////////////////////////////////////////////////////////////////////////////
// main.cpp
// ========
// testing OpenGL Display List
// glGenLists(), glDeleteLists(), glNewList(), glEndList(), glCallList()
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2005-10-04
// UPDATED: 2005-10-12
///////////////////////////////////////////////////////////////////////////////

#include <GL/glut.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <math.h>
#include "asfLidarViz_DL.h"
#include "utility.h"
#include "teapot.h"
#include "pointcloud.h"
#include "Timer.h"
#include "camera.h"
using std::stringstream;
using std::cout;
using std::endl;
using std::ends;


// global variables
void *font = GLUT_BITMAP_8_BY_13;
GLuint listId = 0; // id of display list
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX = 0;
float cameraAngleY = 0;
float cameraDistance;
int drawMode = 0;
Timer timer;
bool dlUsed;

static float ratio;
static float x=0.0f,y=1.75f,z=5.0f;
static float lx=0.0f,ly=0.0f,lz=-1.0f;

float xpos = 0, ypos = 0, zpos = 0, xrot = 0, yrot = 0, zrot = 0, angle=0.0;
float lastx, lasty;

// junk from previous attempt
CCamera Camera;

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    SetupWorld();

    // initialize global variables
    initSharedMem();

    // init GLUT and GL
    initGLUT(argc, argv);
    initGL();


    // compile a display list of teapot mesh
    // see detail in createTeapotDL()
    listId = createPointcloudDL();

    // the last GLUT call (LOOP)
    // window will be shown and display callback is triggered by events
    // NOTE: this call never return main().
    glutMainLoop(); /* Start GLUT event-processing loop */

    return 0;
}

// load in the vertices
void SetupWorld()
{
    float x, y, z;
    GLuint numpoints;
    float max_x, max_y, max_z;
    FILE *filein;        // file to load the world from
    char oneline[255];

    filein = fopen("Data/xyz.txt", "rt");
    //filein = fopen("Data/test2.txt", "rt");

    readstr(filein, oneline);
    sscanf(oneline, "NUMPOINTS %i\n", &numpoints);

    readstr(filein, oneline);
    sscanf(oneline, "MAXX %f\n", &max_x);

    readstr(filein, oneline);
    sscanf(oneline, "MAXY %f\n", &max_y);

    readstr(filein, oneline);
    sscanf(oneline, "MAXZ %f\n", &max_z);

    sector1.numpoints = numpoints;
    sector1.max_x = max_x;
    sector1.max_y = max_y;
    sector1.max_z = max_z;

    sector1.point = (VERTEX *) malloc(sizeof(VERTEX)*numpoints);

    printf("number of points %i\n", numpoints);

    printf("max x %f\n", max_x);
    printf("max y %f\n", max_y);
    printf("max z %f\n", max_z);

    for (GLuint loop = 0; loop < numpoints; loop++) {

        readstr(filein,oneline);
        sscanf(oneline, "%f %f %f", &x, &y, &z);

        //printf("Loaded %f %f %f\n", x, y, z);

        sector1.point[loop].x = x;
        sector1.point[loop].y = y;
        sector1.point[loop].z = z;

        float fColor = (z)/sector1.max_z;
        sector1.point[loop].red = fColor;
        sector1.point[loop].green = fColor/2;
        sector1.point[loop].blue = 1-fColor;
        //printf("  fcolor: %f\n", fColor);
        //printf("  rgb color: %f %f %f\n", 1-fColor, fColor/2, fColor);
    }

    fclose(filein);
    return;
}

GLuint createPointcloudDL()
{
    GLuint id = 0;

    // generate one display list
    id = glGenLists(1);
    if(!id) return id;  // failed to create a list, return 0

    GLuint numpoints = sector1.numpoints;


    // bind vertex and normal pointers
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, sector1.point);

    // store drawing function in the display list =============================
    glNewList(id, GL_COMPILE);
        glPointSize(2);
        glBegin(GL_POINTS);
        for (GLuint loop=0; loop<numpoints; loop++) {        // loop through all the triangles
            GLfloat xm = sector1.point[loop].x;
            GLfloat ym = sector1.point[loop].y;
            GLfloat zm = sector1.point[loop].z;

            glColor3f(sector1.point[loop].red, sector1.point[loop].green, sector1.point[loop].blue);
            glVertex3f( xm, ym, zm);
        }
        glEnd();
    glEndList();    //=========================================================

    glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
    glDisableClientState(GL_NORMAL_ARRAY);  // disable normal arrays

    return id;  // use this for drawing
}

///////////////////////////////////////////////////////////////////////////////
// initialize GLUT for windowing
///////////////////////////////////////////////////////////////////////////////
int initGLUT(int argc, char **argv)
{
    // GLUT stuff for windowing
    // initialization openGL window.
    // it is called before any other GLUT routine
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);   // display mode

    glutInitWindowSize(400, 300);               // window size

    glutInitWindowPosition(100, 100);           // window location

    // finally, create a window with openGL context
    // Window will not displayed until glutMainLoop() is called
    // it returns a unique ID
    int handle = glutCreateWindow(argv[0]);     // param is the title of window

    // register GLUT callback functions
    glutDisplayFunc(displayCB);
    //glutTimerFunc(100, timerCB, 100);           // redraw only every given millisec
    glutIdleFunc(idleCB);                       // redraw when idle
    glutReshapeFunc(reshapeCB);
    glutKeyboardFunc(keyboardCB);
    glutSpecialFunc(inputKey);


    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);

    return handle;
}



///////////////////////////////////////////////////////////////////////////////
// initialize OpenGL
// disable unused features
///////////////////////////////////////////////////////////////////////////////
void initGL()
{
    glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment

    // enable /disable features
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    //glEnable(GL_TEXTURE_2D);
    //glEnable(GL_CULL_FACE);

     // track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)

    glClearColor(0, 0, 0, 0);                   // background color
    //glClearStencil(0);                          // clear stencil buffer
    glClearDepth(1.0f);                         // 0 is near, 1 is far
    glDepthFunc(GL_LEQUAL);


    Camera.Render();
    initLights();



    //setCamera(0, 0, 10, 0, 0, 0);
    //setCamera(1045.0, 580.0, 65.0, 1.0, 1.0, 1.0);
}



///////////////////////////////////////////////////////////////////////////////
// initialize global variables
///////////////////////////////////////////////////////////////////////////////
bool initSharedMem()
{
    mouseLeftDown = mouseRightDown = false;
    dlUsed = true;

    return true;
}



///////////////////////////////////////////////////////////////////////////////
// clean up shared memory
///////////////////////////////////////////////////////////////////////////////
void clearSharedMem()
{
    if(listId)
        glDeleteLists(listId, 1); // delete one
}



///////////////////////////////////////////////////////////////////////////////
// initialize lights
///////////////////////////////////////////////////////////////////////////////
void initLights()
{
    // set up light colors (ambient, diffuse, specular)
    GLfloat lightKa[] = {1.0f, 1.0f, 1.0f, 1.0f};  // ambient light
    GLfloat lightKd[] = {1.0f, 1.0f, 1.0f, 1.0f};  // diffuse light
    GLfloat lightKs[] = {0, 0, 0, 1};           // specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

    // position the light
    float lightPos[4] = {0, 0, 20, 1}; // positional light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);                        // MUST enable each light source after configuration
}



///////////////////////////////////////////////////////////////////////////////
// set camera position and lookat direction
///////////////////////////////////////////////////////////////////////////////
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posX, posY, posZ, targetX, targetY, targetZ, 0, 0, 1); // eye(x,y,z), focal(x,y,z), up(x,y,z)
}



void camera (void) {
    glRotatef(xrot,1.0,0.0,0.0);  //rotate our camera on teh x-axis (left and right)
    glRotatef(yrot,0.0,1.0,0.0);  //rotate our camera on the y-axis (up and down)
    glRotatef(zrot,0.0,0.0,1.0);  //rotate our camera on the z-axis (up and down)
    glTranslated(-xpos,-ypos,-zpos); //translate the screen to the position of our camera
}



//=============================================================================
// CALLBACKS
//=============================================================================

void displayCB()
{
    // clear buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // save the initial ModelView matrix before modifying ModelView matrix
    glPushMatrix();

    camera();


    timer.start();  //=====================================

    glCallList(listId);     // render with display list

    timer.stop();   //=====================================

    // draw info messages
    showInfo();
    showFPS();

    glPopMatrix();

    glutSwapBuffers();
}


void reshapeCB(int w, int h)
{
    // set viewport to be the entire window
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    // set perspective viewing frustum
    //float aspectRatio = (float)w / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glFrustum(-aspectRatio, aspectRatio, -1, 1, 1, 100);
    gluPerspective(60.0f, (float)(w)/h, 1.0f, 1000.0f); // FOV, AspectRatio, NearClip, FarClip

    // switch to modelview matrix in order to set scene
    glMatrixMode(GL_MODELVIEW);
}


void timerCB(int millisec)
{
    glutTimerFunc(millisec, timerCB, millisec);
    glutPostRedisplay();
}


void idleCB()
{
    glutPostRedisplay();
}

void keyboardCB (unsigned char key, int x, int y) {
    if (key=='q')
    {
    zrot -= 1;
    if (zrot >360) zrot -= 360;
    }

    if (key=='e')
    {
    zrot += 1;
    if (zrot < -360) zrot += 360;
    }

    if (key=='z')
    {
    yrot += 1;
    if (yrot >360) yrot -= 360;
    }

    if (key=='c')
    {
    yrot -= 1;
    if (yrot < -360) yrot += 360;
    }

    if (key=='w')
    {
    xpos += 10;

    }

    if (key=='s')
    {
    xpos -= 10;
    }

    if (key=='r')
    {
    zpos += 10;
    }

    if (key=='f')
    {
    zpos -= 10;
    }

    if (key=='d')
    {
    ypos -= 10;
    }

    if (key=='a')
    {
    ypos +=10;
    }

    if (key==27)
    {
    exit(0);
    }
    glutPostRedisplay();
}
/*
void keyboardCB(unsigned char key, int x, int y)
{
    float xrotrad = 0.0;
    float yrotrad = 0.0;
    switch(key)
    {
    case 27: // ESCAPE
        clearSharedMem();
        exit(0);
        break;
    case 'q':
        xrot += 1;
        if(xrot>360) xrot -=360;
    case 'z':
        xrot -= 1;
        if(xrot<360) xrot += 360;
    case 'w':
        yrotrad = (yrot / 180 * 3.141592654f);
        xrotrad = (xrot / 180 * 3.141592654f);
        xpos += float(sin(yrotrad)) ;
        zpos -= float(cos(yrotrad)) ;
        ypos -= float(sin(xrotrad)) ;
    case 's':
        yrotrad = (yrot / 180 * 3.141592654f);
        xrotrad = (xrot / 180 * 3.141592654f);
        xpos -= float(sin(yrotrad));
        zpos += float(cos(yrotrad)) ;
        ypos += float(sin(xrotrad));
    case 'd':
        yrot += 1;
        if (yrot >360) yrot -= 360;
    case 'a':
        yrot -= 1;
        if (yrot < -360)yrot += 360;
    default:
        ;
    }
    glutPostRedisplay();
}
*/

void inputKey(int key, int x, int y) {

    switch (key) {
        case GLUT_KEY_LEFT :
            angle -= 0.01f;
            orientMe(angle);break;
        case GLUT_KEY_RIGHT :
            angle +=0.01f;
            orientMe(angle);break;
        case GLUT_KEY_UP :
            moveMeFlat(1);break;
        case GLUT_KEY_DOWN :
            moveMeFlat(-1);break;
    }
}

void orientMe(float ang) {

    lx = sin(ang);
    lz = -cos(ang);
    glLoadIdentity();
    gluLookAt(x, y, z,
              x + lx,y + ly,z + lz,
              0.0f,1.0f,0.0f);
}
void moveMeFlat(int direction) {
    x = x + direction*(lx)*1.1;
    z = z + direction*(lz)*1.1;
    glLoadIdentity();
    gluLookAt(x, y, z,
              x + lx,y + ly,z + lz,
              0.0f,1.0f,0.0f);
}

/*
void mouseMovement(int x, int y) {
    int diffx=x-lastx; //check the difference between the current x and the last x position
    int diffy=y-lasty; //check the difference between the current y and the last y position
    lastx=x; //set lastx to the current x position
    lasty=y; //set lasty to the current y position
    xrot += (float) diffy; //set the xrot to xrot with the addition of the difference in the y position
    yrot += (float) diffx;    //set the xrot to yrot with the addition of the difference in the x position

    glutPostRedisplay();
}*/

void changeSize(int w, int h)
{

    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero width).
    if(h == 0)
        h = 1;

    ratio = 1.0f * w / h;
    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set the viewport to be the entire window
        glViewport(0, 0, w, h);

    // Set the clipping volume
    gluPerspective(45,ratio,1,1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(x, y, z,
            x + lx,y + ly,z + lz,
            0.0f,1.0f,0.0f);
    }


void mouseCB(int button, int state, int x, int y)
{
    lastx = x;
    lasty = y;

    if(button == GLUT_LEFT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseLeftDown = true;
        }
        else if(state == GLUT_UP)
            mouseLeftDown = false;
    }

    else if(button == GLUT_RIGHT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseRightDown = true;
        }
        else if(state == GLUT_UP)
            mouseRightDown = false;
    }
}


void mouseMotionCB(int x, int y)
{
    if(mouseLeftDown)
    {
        xrot += float(y - lasty);
        yrot += float(x - lastx);
        lastx = x;
        lasty = y;
    }
    if(mouseRightDown)
    {
        zpos += (y - lasty);  //move up and down
        zrot += (x - lastx);  //rotate around
        lasty = y;
        lastx = x;
    }

    glutPostRedisplay();
}


///////////////////////////////////////////////////////////////////////////////
// display info messages
///////////////////////////////////////////////////////////////////////////////
void showInfo()
{
    // backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    // set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);     // switch to projection matrix
    glPushMatrix();                  // save current projection matrix
    glLoadIdentity();                // reset projection matrix
    gluOrtho2D(0, 400, 0, 300);  // set to orthogonal projection

    float color[4] = {1, 1, 1, 1};

    stringstream ss;
    ss << std::fixed << std::setprecision(3);

    ss << "Display List: " << (dlUsed ? "on" : "off") << ends;
    drawString(ss.str().c_str(), 1, 286, color, font);
    ss.str("");

    // display elapsed time in millisec
    ss << "Time: " << timer.getElapsedTimeInMilliSec() << " ms" << ends;
    drawString(ss.str().c_str(), 1, 272, color, font);
    ss.str("");


    // unset floating format
    ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);

    // restore projection matrix
    glPopMatrix();                   // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);      // switch to modelview matrix
    glPopMatrix();                   // restore to previous modelview matrix
}



///////////////////////////////////////////////////////////////////////////////
// display frame rates
///////////////////////////////////////////////////////////////////////////////
void showFPS()
{
    static Timer timer;
    static int count = 0;
    static stringstream ss;
    double elapsedTime;

    // backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    // set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);        // switch to projection matrix
    glPushMatrix();                     // save current projection matrix
    glLoadIdentity();                   // reset projection matrix
    gluOrtho2D(0, 400, 0, 300);         // set to orthogonal projection

    float color[4] = {1, 1, 0, 1};

    // update fps every second
    elapsedTime = timer.getElapsedTime();
    if(elapsedTime < 1.0)
    {
        ++count;
    }
    else
    {
        ss.str("");
        ss << std::fixed << std::setprecision(1);
        ss << (count / elapsedTime) << " FPS" << ends; // update fps string
        ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
        count = 0;                      // reset counter
        timer.start();                  // restart timer
    }
    drawString(ss.str().c_str(), 315, 286, color, font);

    // restore projection matrix
    glPopMatrix();                      // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);         // switch to modelview matrix
    glPopMatrix();                      // restore to previous modelview matrix
}
