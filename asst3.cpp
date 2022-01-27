////////////////////////////////////////////////////////////////////////
//
//   Harvard University
//   CS175 : Computer Graphics
//   Professor Steven Gortler
//
////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#include <GL/glew.h>
#ifdef __MAC__
#   include <GLUT/glut.h>
#else
#   include <GL/glut.h>
#endif

#include "cvec.h"
#include "matrix4.h"
#include "rigtform.h"
#include "arcball.h"
#include "geometrymaker.h"
#include "ppm.h"
#include "glsupport.h"

using namespace std;      // for string, vector, iostream, and other standard C++ stuff

// G L O B A L S ///////////////////////////////////////////////////

// --------- IMPORTANT --------------------------------------------------------
// Before you start working on this assignment, set the following variable
// properly to indicate whether you want to use OpenGL 2.x with GLSL 1.0 or
// OpenGL 3.x+ with GLSL 1.3.
//
// Set g_Gl2Compatible = true to use GLSL 1.0 and g_Gl2Compatible = false to
// use GLSL 1.3. Make sure that your machine supports the version of GLSL you
// are using. In particular, on Mac OS X currently there is no way of using
// OpenGL 3.x with GLSL 1.3 when GLUT is used.
//
// If g_Gl2Compatible=true, shaders with -gl2 suffix will be loaded.
// If g_Gl2Compatible=false, shaders with -gl3 suffix will be loaded.
// To complete the assignment you only need to edit the shader files that get
// loaded
// ----------------------------------------------------------------------------
static const bool g_Gl2Compatible = true;


static const float g_frustMinFov = 60.0;  // A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov; // FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -0.1;    // near plane
static const float g_frustFar = -50.0;    // far plane
static const float g_groundY = -2.0;      // y coordinate of the ground
static const float g_groundSize = 10.0;   // half the ground length

static int g_windowWidth = 512;
static int g_windowHeight = 512;
static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;

struct ShaderState {
  GlProgram program;

  // Handles to uniform variables
  GLint h_uLight, h_uLight2;
  GLint h_uProjMatrix;
  GLint h_uModelViewMatrix;
  GLint h_uNormalMatrix;
  GLint h_uColor;

  // Handles to vertex attributes
  GLint h_aPosition;
  GLint h_aNormal;

  ShaderState(const char* vsfn, const char* fsfn) {
    readAndCompileShader(program, vsfn, fsfn);

    const GLuint h = program; // short hand

    // Retrieve handles to uniform variables
    h_uLight = safe_glGetUniformLocation(h, "uLight");
    h_uLight2 = safe_glGetUniformLocation(h, "uLight2");
    h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
    h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
    h_uNormalMatrix = safe_glGetUniformLocation(h, "uNormalMatrix");
    h_uColor = safe_glGetUniformLocation(h, "uColor");

    // Retrieve handles to vertex attributes
    h_aPosition = safe_glGetAttribLocation(h, "aPosition");
    h_aNormal = safe_glGetAttribLocation(h, "aNormal");

    if (!g_Gl2Compatible)
      glBindFragDataLocation(h, 0, "fragColor");
    checkGlErrors();
  }

};

static const int g_numShaders = 2;
static const char * const g_shaderFiles[g_numShaders][2] = {
  {"./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader"},
  {"./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader"}
};
static const char * const g_shaderFilesGl2[g_numShaders][2] = {
  {"./shaders/basic-gl2.vshader", "./shaders/diffuse-gl2.fshader"},
  {"./shaders/basic-gl2.vshader", "./shaders/solid-gl2.fshader"}
};
static vector<shared_ptr<ShaderState> > g_shaderStates; // our global shader states

// --------- Geometry

// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) &(((StructType *)0)->field)

// A vertex with floating point position and normal
struct VertexPN {
  Cvec3f p, n;

  VertexPN() {}
  VertexPN(float x, float y, float z,
           float nx, float ny, float nz)
    : p(x,y,z), n(nx, ny, nz)
  {}

  // Define copy constructor and assignment operator from GenericVertex so we can
  // use make* functions from geometrymaker.h
  VertexPN(const GenericVertex& v) {
    *this = v;
  }

  VertexPN& operator = (const GenericVertex& v) {
    p = v.pos;
    n = v.normal;
    return *this;
  }
};

struct Geometry {
  GlBufferObject vbo, ibo;
  int vboLen, iboLen;

  Geometry(VertexPN *vtx, unsigned short *idx, int vboLen, int iboLen) {
    this->vboLen = vboLen;
    this->iboLen = iboLen;

    // Now create the VBO and IBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPN) * vboLen, vtx, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboLen, idx, GL_STATIC_DRAW);
  }

  void draw(const ShaderState& curSS) {
    // Enable the attributes used by our shader
    safe_glEnableVertexAttribArray(curSS.h_aPosition);
    safe_glEnableVertexAttribArray(curSS.h_aNormal);

    // bind vbo
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    safe_glVertexAttribPointer(curSS.h_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), FIELD_OFFSET(VertexPN, p));
    safe_glVertexAttribPointer(curSS.h_aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), FIELD_OFFSET(VertexPN, n));

    // bind ibo
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    // draw!
    glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);

    // Disable the attributes used by our shader
    safe_glDisableVertexAttribArray(curSS.h_aPosition);
    safe_glDisableVertexAttribArray(curSS.h_aNormal);
  }
};


// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_cube2, g_arcball;

// --------- Scene
static const int g_numObjects = 2;
static const int g_numViews = g_numObjects + 1;     // plus 1 because of the sky

static int g_curViewIdx = 0;    // 0: sky, 1: red cube, 2: blue cube

static const Cvec3 g_light1(2.0, 3.0, 14.0), g_light2(-2, -3.0, -5.0);  // define two lights positions in world space

static RigTForm g_skyRbt = RigTForm(Cvec3(0.0, 0.25, 4.0));

static RigTForm g_objectRbt[g_numObjects] = {
        RigTForm(Cvec3(-1,0,0)),
        RigTForm(Cvec3(1,0,0)),
};
static Cvec3f g_objectColors[] = {
        Cvec3f(1, 0, 0),
        Cvec3f(0, 0, 1),
};

static RigTForm g_Frame = linFact(g_skyRbt);    // default to world-sky frame

static int g_curManipulatingObjIdx = 0;        // 0: sky, 1: red cube, 2: blue cube

enum SkyFrame : int {
    World_Sky,
    Sky_Sky,
};

static SkyFrame g_curSkyFrame = World_Sky;


// arcball
static const Cvec3f g_arcballColor = Cvec3f(1, 1, 1);
static const int g_arcballSlices = 20;
static const int g_arcballStacks = 20;

static double g_arcballScreenRadiusFactor = 0.25;
static double g_arcballScreenRadius = g_arcballScreenRadiusFactor * min(g_windowWidth, g_windowHeight);
static double g_arcballScale;


///////////////// END OF G L O B A L S //////////////////////////////////////////////////




static void initGround() {
  // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
  VertexPN vtx[4] = {
    VertexPN(-g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
    VertexPN(-g_groundSize, g_groundY,  g_groundSize, 0, 1, 0),
    VertexPN( g_groundSize, g_groundY,  g_groundSize, 0, 1, 0),
    VertexPN( g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
  };
  unsigned short idx[] = {0, 1, 2, 0, 2, 3};
  g_ground.reset(new Geometry(&vtx[0], &idx[0], 4, 6));
}

static void initCubes() {
  int ibLen, vbLen;
  getCubeVbIbLen(vbLen, ibLen);

  // Temporary storage for cube geometry
  vector<VertexPN> vtx(vbLen);
  vector<unsigned short> idx(ibLen);

  makeCube(1, vtx.begin(), idx.begin());
  g_cube.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));

  vector<VertexPN> vtx2(vbLen);
  vector<unsigned short> idx2(ibLen);

  makeCube(1, vtx2.begin(), idx2.begin());
  g_cube2.reset(new Geometry(&vtx2[0], &idx2[0], vbLen, ibLen));
}

static void initArcball() {
    int ibLen, vbLen;
    getSphereVbIbLen(g_arcballSlices, g_arcballStacks, vbLen, ibLen);

    // Temporary storage for arcball geometry
    vector<VertexPN> vtx(vbLen);
    vector<unsigned short> idx(ibLen);

    makeSphere(1, g_arcballSlices, g_arcballStacks, vtx.begin(), idx.begin());
    g_arcball.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));
}

// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(const ShaderState& curSS, const Matrix4& projMatrix) {
  GLfloat glmatrix[16];
  projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
  safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}

// takes MVM and its normal matrix to the shaders
static void sendModelViewNormalMatrix(const ShaderState& curSS, const Matrix4& MVM, const Matrix4& NMVM) {
  GLfloat glmatrix[16];
  MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
  safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

  NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
  safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);
}

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
  if (g_windowWidth >= g_windowHeight)
    g_frustFovY = g_frustMinFov;
  else {
    const double RAD_PER_DEG = 0.5 * CS175_PI/180;
    g_frustFovY = atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight / g_windowWidth, cos(g_frustMinFov * RAD_PER_DEG)) / RAD_PER_DEG;
  }
}

static Matrix4 makeProjectionMatrix() {
  return Matrix4::makeProjection(
           g_frustFovY, g_windowWidth / static_cast <double> (g_windowHeight),
           g_frustNear, g_frustFar);
}

static bool isUsingArcball() {
    if (g_curManipulatingObjIdx == 0 && g_curSkyFrame == World_Sky) {
        return true;
    } else {
        if (g_curManipulatingObjIdx != g_curViewIdx)
            return true;
        else
            return false;
    }
}

static void drawStuff() {
  // short hand for current shader state
  const ShaderState& curSS = *g_shaderStates[g_activeShader];

  // build & send proj. matrix to vshader
  const Matrix4 projmat = makeProjectionMatrix();
  sendProjectionMatrix(curSS, projmat);

  // choose the correct Rbt as eyeRbt
  const RigTForm eyeRbt = (g_curViewIdx == 0) ? g_skyRbt : g_objectRbt[g_curViewIdx - 1];

  const RigTForm invEyeRbt = inv(eyeRbt);

  const Cvec3 eyeLight1 = Cvec3(invEyeRbt * Cvec4(g_light1, 1)); // g_light1 position in eye coordinates
  const Cvec3 eyeLight2 = Cvec3(invEyeRbt * Cvec4(g_light2, 1)); // g_light2 position in eye coordinates
  safe_glUniform3f(curSS.h_uLight, eyeLight1[0], eyeLight1[1], eyeLight1[2]);
  safe_glUniform3f(curSS.h_uLight2, eyeLight2[0], eyeLight2[1], eyeLight2[2]);

  // draw ground
  // ===========
  //
  const RigTForm groundRbt = RigTForm();  // identity
  RigTForm MVRigTForm = invEyeRbt * groundRbt;
  Matrix4 MVM = rigTFormToMatrix(MVRigTForm);
  Matrix4 NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, 0.1, 0.95, 0.1); // set color
  g_ground->draw(curSS);

  // draw cubes
  // ==========
  MVRigTForm = invEyeRbt * g_objectRbt[0];
  MVM = rigTFormToMatrix(MVRigTForm);
  NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, g_objectColors[0][0], g_objectColors[0][1], g_objectColors[0][2]);
  g_cube->draw(curSS);

  MVRigTForm = invEyeRbt * g_objectRbt[1];
  MVM = rigTFormToMatrix(MVRigTForm);
  NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, g_objectColors[1][0], g_objectColors[1][1], g_objectColors[1][2]);
  g_cube2->draw(curSS);

  // draw arcball in wireframe mode
  // ==========
  if (!isUsingArcball())
      return;

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // choose the correct rbt according to current manipulating object and current view
  RigTForm tmpRbt;
  if (g_curManipulatingObjIdx == 0) {   // manipulating sky
      if (g_curSkyFrame == World_Sky) {
          tmpRbt = RigTForm();
      } else {
          tmpRbt = eyeRbt;
      }
  } else {  // manipulating cubes
      tmpRbt = g_objectRbt[g_curManipulatingObjIdx - 1];
  }

  MVRigTForm = invEyeRbt * tmpRbt;

  // when we are not translating along z axis, update g_arcballScale
  if (!g_mouseMClickButton && !(g_mouseLClickButton && g_mouseRClickButton)) {
      g_arcballScale = getScreenToEyeScale(
              (invEyeRbt * tmpRbt).getTranslation()[2],
              g_frustFovY,
              g_windowHeight);
  }

  // remember to scale correct the arcball
  MVM = rigTFormToMatrix(MVRigTForm) * Matrix4::makeScale(g_arcballScale * g_arcballScreenRadius);
  NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, g_arcballColor[0], g_arcballColor[1], g_arcballColor[2]);
  g_arcball->draw(curSS);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void display() {
  glUseProgram(g_shaderStates[g_activeShader]->program);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                   // clear framebuffer color&depth

  drawStuff();

  glutSwapBuffers();                                    // show the back buffer (where we rendered stuff)

  checkGlErrors();
}

static void reshape(const int w, const int h) {
  g_windowWidth = w;
  g_windowHeight = h;
  glViewport(0, 0, w, h);
  cerr << "Size of window is now " << w << "x" << h << endl;
  g_arcballScreenRadius = g_arcballScreenRadiusFactor * min(g_windowWidth, g_windowHeight);
  updateFrustFovY();
  glutPostRedisplay();
}

static void setFrame() {
    if (g_curManipulatingObjIdx == 0) { // manipulating sky
        if (g_curViewIdx == 0) {    // view is sky
            if (g_curSkyFrame == World_Sky) {
                g_Frame = linFact(g_skyRbt);    // world-sky frame
            } else {
                g_Frame = g_skyRbt; // sky-sky frame
            }
        }
    } else {    // manipulating cube
        if (g_curViewIdx == 0) {    // view is sky
            g_Frame = transFact(g_objectRbt[g_curManipulatingObjIdx - 1]) * linFact(g_skyRbt);
        } else {    // view is cube
            g_Frame = transFact(g_objectRbt[g_curManipulatingObjIdx - 1]) * linFact(g_objectRbt[g_curViewIdx - 1]);
        }
    }
}

static void motion(const int x, const int y) {
  // Should not allow modifying the sky camera when the current camera view is a cube view.
  if (g_curViewIdx != 0 && g_curManipulatingObjIdx == 0) return;

  const double raw_dx = x - g_mouseClickX;
  const double raw_dy = g_windowHeight - y - 1 - g_mouseClickY;

  /* invert dx and/or dy depending on the situation */
  double dx_translate, dx_rotate, dy_trans, dy_rotate;
  if (g_curManipulatingObjIdx != 0 && g_curViewIdx != g_curManipulatingObjIdx) {
      /* manipulating cube, and view not from that cube */
      dx_translate = raw_dx;
      dy_trans = raw_dy;
      dx_rotate = raw_dx;
      dy_rotate = raw_dy;
  } else if (g_curManipulatingObjIdx == 0 && g_curViewIdx == 0 && g_curSkyFrame == World_Sky) {
      /* manipulating sky camera, while eye is sky camera, and while in world-sky mode */
      dx_translate = -raw_dx;
      dy_trans = -raw_dy;
      dx_rotate = -raw_dx;
      dy_rotate = -raw_dy;
  } else {
      dx_translate = raw_dx;
      dy_trans = raw_dy;
      dx_rotate = -raw_dx;
      dy_rotate = -raw_dy;
  }

  setFrame();

  const double translateFactor = isUsingArcball()? g_arcballScale : 0.01;

  RigTForm rigTForm;
  if (g_mouseLClickButton && !g_mouseRClickButton) { // left button down?
      rigTForm = RigTForm(Quat::makeXRotation(-dy_rotate) * Quat::makeYRotation(dx_rotate));
  }
  else if (g_mouseRClickButton && !g_mouseLClickButton) { // right button down?
      rigTForm = RigTForm(Cvec3(dx_translate, dy_trans, 0) * translateFactor);
  }
  else if (g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton)) {  // middle or (left and right) button down?
      rigTForm = RigTForm(Cvec3(0, 0, -dy_trans) * translateFactor);
  }

  if (g_mouseClickDown) {
      if (g_curManipulatingObjIdx == 0) {
          g_skyRbt = g_Frame * rigTForm * inv(g_Frame) * g_skyRbt;
      } else {
          g_objectRbt[g_curManipulatingObjIdx - 1] = g_Frame * rigTForm * inv(g_Frame) * g_objectRbt[g_curManipulatingObjIdx - 1];
      }
  }

  glutPostRedisplay(); // we always redraw if we changed the scene

  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1;
}


static void mouse(const int button, const int state, const int x, const int y) {
  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1;  // conversion from GLUT window-coordinate-system to OpenGL window-coordinate-system

  g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
  g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
  g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

  g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
  g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
  g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

  g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;

  // Add this to support the snap effect
  glutPostRedisplay();
}

static void cycleView() {
    g_curViewIdx = (g_curViewIdx + 1) % g_numViews;

    if (g_curViewIdx == 0) {
        cout << "View from sky" << endl;
    } else {
        cout << "View from object " << g_curViewIdx << endl;
    }
}

static void cycleManipulationMode() {
    g_curManipulatingObjIdx = (g_curManipulatingObjIdx + 1) % g_numViews;

    if (g_curManipulatingObjIdx == 0) {
        cout << "Manipulating sky" << endl;
    } else {
        cout << "Manipulating object " << g_curManipulatingObjIdx << endl;
    }
}

static void cycleSkyAMatrix() {
    if (g_curManipulatingObjIdx == 0 && g_curViewIdx == 0) {
        g_curSkyFrame = static_cast<SkyFrame>((g_curSkyFrame + 1) % 2);     // toggle between World_Sky and Sky_Sky
        if (g_curSkyFrame == World_Sky) {
            cout << "Using world-sky frame" << endl;
        } else {
            cout << "Using sky-sky frame" << endl;
        }
    } else {
        cout << "Unable to change sky manipulation mode while in current view." << endl;
    }
}

static void keyboard(const unsigned char key, const int x, const int y) {
  switch (key) {
  case 27:
    exit(0);                                  // ESC
  case 'h':
    cout << " ============== H E L P ==============\n\n"
    << "h\t\thelp menu\n"
    << "s\t\tsave screenshot\n"
    << "f\t\tToggle flat shading on/off.\n"
    << "o\t\tCycle object to edit\n"
    << "v\t\tCycle view\n"
    << "drag left mouse to rotate\n" << endl;
    break;
  case 's':
    glFlush();
    writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
    break;
  case 'f':
    g_activeShader ^= 1;
    break;
  case 'v':
      cycleView();
      break;
  case 'o':
      cycleManipulationMode();
      break;
  case 'm':
      cycleSkyAMatrix();
      break;
  }
  glutPostRedisplay();
}

static void initGlutState(int argc, char * argv[]) {
  glutInit(&argc, argv);                                  // initialize Glut based on cmd-line args
  glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);  //  RGBA pixel channels and double buffering
  glutInitWindowSize(g_windowWidth, g_windowHeight);      // create a window
  glutCreateWindow("Assignment 3");                       // title the window

  glutDisplayFunc(display);                               // display rendering callback
  glutReshapeFunc(reshape);                               // window reshape callback
  glutMotionFunc(motion);                                 // mouse movement callback
  glutMouseFunc(mouse);                                   // mouse click callback
  glutKeyboardFunc(keyboard);
}

static void initGLState() {
  glClearColor(128./255., 200./255., 255./255., 0.);
  glClearDepth(0.);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glReadBuffer(GL_BACK);
  if (!g_Gl2Compatible)
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initShaders() {
  g_shaderStates.resize(g_numShaders);
  for (int i = 0; i < g_numShaders; ++i) {
    if (g_Gl2Compatible)
      g_shaderStates[i].reset(new ShaderState(g_shaderFilesGl2[i][0], g_shaderFilesGl2[i][1]));
    else
      g_shaderStates[i].reset(new ShaderState(g_shaderFiles[i][0], g_shaderFiles[i][1]));
  }
}

static void initGeometry() {
  initGround();
  initCubes();
  initArcball();
}

int main(int argc, char * argv[]) {
  try {
    initGlutState(argc,argv);

    glewInit(); // load the OpenGL extensions

    cout << (g_Gl2Compatible ? "Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.3") << endl;
    if ((!g_Gl2Compatible) && !GLEW_VERSION_3_0)
      throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.3");
    else if (g_Gl2Compatible && !GLEW_VERSION_2_0)
      throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.0");

    initGLState();
    initShaders();
    initGeometry();

    glutMainLoop();
    return 0;
  }
  catch (const runtime_error& e) {
    cout << "Exception caught: " << e.what() << endl;
    return -1;
  }
}
