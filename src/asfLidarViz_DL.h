// GLUT CALLBACK functions
void displayCB();
void reshapeCB(int w, int h);
void timerCB(int millisec);
void idleCB();
void keyboardCB(unsigned char key, int x, int y);
void mouseCB(int button, int stat, int x, int y);
void mouseMotionCB(int x, int y);

void initGL();
int  initGLUT(int argc, char **argv);
bool initSharedMem();
void clearSharedMem();
void initLights();
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ);

void showInfo();
void showFPS();
void SetupWorld();
GLuint createPointcloudDL();
void inputKey(int, int, int);
void reshape(int, int);
void orientMe(float);
void moveMeFlat(int);
void camera (void);

