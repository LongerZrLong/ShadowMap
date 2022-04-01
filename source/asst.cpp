////////////////////////////////////////////////////////////////////////
//
//   Harvard University
//   CS175 : Computer Graphics
//   Professor Steven Gortler
//
////////////////////////////////////////////////////////////////////////

#include <memory>
#include <stdexcept>
#include <vector>

#include <GL/glew.h>
#ifdef __MAC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "arcball.h"
#include "cvec.h"
#include "geometrymaker.h"
#include "glsupport.h"
#include "matrix4.h"
#include "ppm.h"
#include "rigtform.h"

#include "asstcommon.h"
#include "scenegraph.h"
#include "drawer.h"
#include "picker.h"

#include "script.h"

#include "geometry.h"

#include "tiny_obj_loader.h"

using namespace std;// for string, vector, iostream, and other standard C++ stuff

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
// If g_Gl2Compatible=false, shaders with -gl2 suffix will be loaded.
// To complete the assignment you only need to edit the shader files that get
// loaded
// ----------------------------------------------------------------------------
const bool g_Gl2Compatible = true;

static const string SCRIPT_PATH = "./resource/script.txt" ;
static const string OBJ_FILE_PATH = "./resource/girl_model/girl.obj";
static const string TEX_FILE_PATH = "./resource/girl_model/MC003_Kozakura_Mari.png";

enum Key : int {
  Esc = 27,
  Space = 32,
};

static const float g_frustMinFov = 60.0; // A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov;// FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -1.0; // near plane
static const float g_frustFar = -100.0; // far plane
static const float g_groundY = -2.0;   // y coordinate of the ground
static const float g_planeSize = 6.0;// half the ground length

static int g_windowWidth = 1024;
static int g_windowHeight = 768;
static bool g_mouseClickDown = false;// is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY;// coordinates for mouse click event

static bool g_isPicking = false;


// --------- Material
static shared_ptr<Material>
        g_redDiffuseMat,
        g_blueDiffuseMat,
        g_greenDiffuseMat,
        g_grayDiffuseMat,
        g_arcballMat,
        g_pickingMat,
        g_lightMat,
        g_girlMat;

static shared_ptr<Material> g_debugDepthMat;

shared_ptr<Material> g_overridingMaterial;
shared_ptr<Material> g_shadowPassMat;


// --------- Geometry

typedef SgGeometryShapeNode MyShapeNode;

// Vertex buffer and index buffer associated with the ground, cube, sphere, quad geometry
static shared_ptr<Geometry> g_plane, g_cube, g_sphere, g_quad;


// --------- Scene
static shared_ptr<SgRootNode> g_world;
static shared_ptr<SgRbtNode> g_skyNode, g_boxNode;
static shared_ptr<SgRbtNode> g_lightNode;
static shared_ptr<SgRbtNode> g_robot1Node;
static shared_ptr<SgRbtNode> g_girlNode;

static const int g_numObjects = 2;
static const int g_numViews = g_numObjects + 1;// plus 1 because of the sky

static int g_curViewIdx = 0;// 0: sky, 1: robot1, 2: robot2

static shared_ptr<SgRbtNode> g_currentPickedRbtNode;
static shared_ptr<SgRbtNode> g_currentViewRbtNode;

static RigTForm g_Frame;

enum SkyFrame : int {
  World_Sky,
  Sky_Sky,
};

static SkyFrame g_curSkyFrame = World_Sky;


// arcball
static bool g_isUsingArcball = true;
static double g_arcballScreenRadiusFactor = 0.1;
static double g_arcballScreenRadius = g_arcballScreenRadiusFactor * min(g_windowWidth, g_windowHeight);
static double g_arcballScale;
static bool g_useWorldSkyArcball = false;


// script
static Script g_script;

static int g_msBetweenKeyFrames = 2000; // 2 seconds between keyframes
static int g_animateFramesPerSecond = 60; // frames to render per second during animation playback

static bool g_isPlaying = false;

// shadow
static shared_ptr<GlFramebuffer> g_depthMapFbo;
static shared_ptr<Texture> g_depthMap;

bool g_isShadowPass = false;

static const int g_shadowTexWidth = 2048;
static const int g_shadowTexHeight = 2048;

static const float g_dirLightHalfArea = 2.0f;
static const float g_dirLightNear = 1.0f;
static const float g_dirLightFar = -1.f;


///////////////// END OF G L O B A L S //////////////////////////////////////////////////


static void initPlane() {
  int ibLen, vbLen;
  getPlaneVbIbLen(vbLen, ibLen);

  // Temporary storage for cube Geometry
  vector<VertexPNTBX> vtx(vbLen);
  vector<unsigned short> idx(ibLen);

  makePlane(g_planeSize *2, vtx.begin(), idx.begin());
  g_plane.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vbLen, ibLen));
}

static void initCubes() {
  int ibLen, vbLen;
  getCubeVbIbLen(vbLen, ibLen);

  // Temporary storage for cube Geometry
  vector<VertexPNTBX> vtx(vbLen);
  vector<unsigned short> idx(ibLen);

  makeCube(1, vtx.begin(), idx.begin());
  g_cube.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vbLen, ibLen));
}

static void initSphere() {
  int ibLen, vbLen;
  getSphereVbIbLen(20, 10, vbLen, ibLen);

  // Temporary storage for sphere Geometry
  vector<VertexPNTBX> vtx(vbLen);
  vector<unsigned short> idx(ibLen);
  makeSphere(1, 20, 10, vtx.begin(), idx.begin());
  g_sphere.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vtx.size(), idx.size()));
}

static void initQuad() {
  // Temporary storage for quad Geometry
  VertexPX vtx[4] = {
          VertexPX(-1, -1, 0, 0, 0),
          VertexPX(1, -1, 0, 1, 0),
          VertexPX(1, 1, 0, 1, 1),
          VertexPX(-1, 1, 0, 0, 1)
  };

  unsigned short idx[6] = {0, 1, 2, 2, 3, 0};

  g_quad.reset(new SimpleIndexedGeometryPX(vtx, idx, 4, 6));
}

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
  if (g_windowWidth >= g_windowHeight)
    g_frustFovY = g_frustMinFov;
  else {
    const double RAD_PER_DEG = 0.5 * CS175_PI / 180;
    g_frustFovY = atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight / g_windowWidth, cos(g_frustMinFov * RAD_PER_DEG)) / RAD_PER_DEG;
  }
}

static Matrix4 makeProjectionMatrix() {
  return Matrix4::makeProjection(
          g_frustFovY, g_windowWidth / static_cast<double>(g_windowHeight),
          g_frustNear, g_frustFar);
}

static Matrix4 makeLightProjectionMatrix() {
  return Matrix4::makeProjection(g_dirLightHalfArea, -g_dirLightHalfArea,
                                 -g_dirLightHalfArea, g_dirLightHalfArea,
                                 g_dirLightNear, g_dirLightFar);
}

static bool isUsingArcball() {
  if (!g_isUsingArcball) {
    return false;
  }

  if (g_useWorldSkyArcball && *g_currentPickedRbtNode == *g_skyNode && g_curSkyFrame == World_Sky) {
    return true;
  }

  if (*g_currentPickedRbtNode != *g_skyNode && *g_currentViewRbtNode != *g_currentPickedRbtNode) {
    return true;
  }

  return false;
}

static void setFrame() {
  if (*g_currentPickedRbtNode == *g_skyNode) {// pick sky
    if (*g_currentViewRbtNode == *g_skyNode) {// view is sky
      if (g_curSkyFrame == World_Sky) {
        g_Frame = linFact(g_skyNode->getRbt()); // world-sky frame
      } else {
        g_Frame = g_skyNode->getRbt();  // sky-sky frame
      }
    }
  } else {// pick other
    if (*g_currentViewRbtNode == *g_skyNode) {// view is sky
      g_Frame = inv(getPathAccumRbt(g_world, g_currentPickedRbtNode, 1))
                * transFact(getPathAccumRbt(g_world, g_currentPickedRbtNode))
                * linFact(getPathAccumRbt(g_world, g_skyNode));
      } else {// view is other
        g_Frame = inv(getPathAccumRbt(g_world, g_currentPickedRbtNode, 1))
                  * getPathAccumRbt(g_world, g_currentPickedRbtNode);
      }
  }
}

static void drawArcball(Uniforms &uniforms, const RigTForm &viewRbt) {

  RigTForm MVRigTForm;
  if (*g_currentPickedRbtNode == *g_skyNode) {  // pick sky
    if (g_curSkyFrame == World_Sky) {
      MVRigTForm = viewRbt;
    } else {
      MVRigTForm = RigTForm();
    }
  } else {  // pick other
    MVRigTForm = viewRbt * getPathAccumRbt(g_world, g_currentPickedRbtNode);
  }

  if (MVRigTForm.getTranslation()[2] > g_frustNear) // if arcball's center is behind z near, don't draw it
    return;

  // when we are not translating along z axis, update g_arcballScale
  if (!g_mouseMClickButton && !(g_mouseLClickButton && g_mouseRClickButton)) {
    g_arcballScale = getScreenToEyeScale(
            MVRigTForm.getTranslation()[2],
            g_frustFovY,
            g_windowHeight);
  }

  // remember to scale correct the arcball
  Matrix4 MVM = rigTFormToMatrix(MVRigTForm) * Matrix4::makeScale(g_arcballScale * g_arcballScreenRadius);
  sendModelViewNormalMatrix(uniforms, MVM, normalMatrix(MVM));

  g_arcballMat->draw(*g_sphere, uniforms);
}

static void drawDepth() {
  Uniforms uniforms;

  // build & send proj. matrix to vshader
  const Matrix4 projmat = makeLightProjectionMatrix();
  sendProjectionMatrix(uniforms, projmat);

  // obtain lightViewRbt
  const RigTForm eyeRbt = getPathAccumRbt(g_world, g_lightNode);
  const Cvec3 lightPos = Cvec3(eyeRbt * Cvec4(0, 0, 0, 1));

  Matrix4 lightView = Matrix4::lookAt(lightPos, Cvec3(0, 0, 0), Cvec3(0, 1, 0));
  RigTForm lightViewRbt = matrixToRigTForm(lightView);

  uniforms.put("uEyeLightMatrix", Matrix4());
  uniforms.put("uLightProjMatrix", Matrix4());

  Drawer drawer(lightViewRbt, uniforms);
  g_world->accept(drawer);
}

static void drawStuff(bool picking) {
  Uniforms uniforms;

  // build & send proj. matrix to vshader
  const Matrix4 projmat = makeProjectionMatrix();
  sendProjectionMatrix(uniforms, projmat);

  // set eyeRbt
  const RigTForm eyeRbt = getPathAccumRbt(g_world, g_currentViewRbtNode);
  const RigTForm invEyeRbt = inv(eyeRbt);

  // light
  const RigTForm lightRbt = getPathAccumRbt(g_world, g_lightNode);
  const RigTForm invLightRbt = inv(lightRbt);

  const Cvec3 eyeLight = Cvec3(invEyeRbt * Cvec4(lightRbt.getTranslation(), 1));

  const Cvec3 lightPos = Cvec3(lightRbt * Cvec4(0,0,0, 1));
  Matrix4 lightView = Matrix4::lookAt(lightPos, Cvec3(0, 0, 0), Cvec3(0, 1, 0));
  RigTForm lightViewRbt = matrixToRigTForm(lightView);

  const RigTForm eyeLightRbt = lightViewRbt * eyeRbt;
  const Matrix4 lightProj = makeLightProjectionMatrix();

  // send the eye space coordinates of lights to uniforms
  uniforms.put("uLight", eyeLight);
  uniforms.put("uEyeLightMatrix", rigTFormToMatrix(eyeLightRbt));
  uniforms.put("uLightProjMatrix", lightProj);
  uniforms.put("uDepthMap", g_depthMap);

  setFrame();

  if (!picking) {
    // initialize the drawer with our uniforms, as opposed to curSS
    Drawer drawer(invEyeRbt, uniforms);

    g_world->accept(drawer);

    // draw arcball in wireframe mode
    // ==========
    if (isUsingArcball())
      drawArcball(uniforms, invEyeRbt);

  } else {
    // intialize the picker with our uniforms, as opposed to curSS
    Picker picker(invEyeRbt, uniforms);

    // set overiding material to our picking material
    g_overridingMaterial = g_pickingMat;

    g_world->accept(picker);

    // unset the overriding material
    g_overridingMaterial.reset();

    glFlush();
    g_currentPickedRbtNode = picker.getRbtNodeAtXY(g_mouseClickX, g_mouseClickY);
    if (*g_currentPickedRbtNode == *g_boxNode || g_currentPickedRbtNode == nullptr)
      g_currentPickedRbtNode = g_skyNode;
  }

}

static void pick() {
  // We need to set the clear color to black, for pick rendering.
  // so let's save the clear color
  GLdouble clearColor[4];
  glGetDoublev(GL_COLOR_CLEAR_VALUE, clearColor);

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  drawStuff(true);

  // Uncomment below and comment out the glutPostRedisplay in mouse(...) call back
  // to see result of the pick rendering pass
  // glutSwapBuffers();

  //Now set back the clear color
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

  checkGlErrors();
}

static void display() {
  {
    // shadow pass
    g_isShadowPass = true;
    glBindFramebuffer(GL_FRAMEBUFFER, g_depthMapFbo);
    glViewport(0, 0, g_shadowTexWidth, g_shadowTexHeight);

    glClear(GL_DEPTH_BUFFER_BIT);

    drawDepth();

    // bind the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_windowWidth, g_windowHeight);
    g_isShadowPass = false;
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  drawStuff(false);

  {
    // draw the quad for debugging depth map
    glViewport(0, 0, g_windowWidth / 4, g_windowHeight / 4);
    glDisable(GL_DEPTH_TEST);

    Uniforms uniforms;
    uniforms.put("uDepthMap", g_depthMap);
    uniforms.put("uNearPlane", g_frustNear);
    uniforms.put("uFarPlane", g_frustFar);
    g_debugDepthMat->draw(*g_quad, uniforms);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, g_windowWidth, g_windowHeight);
  }

  glutSwapBuffers();

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

static void motion(const int x, const int y) {
  // Should not allow modifying the sky camera when the current camera view is not the sky view.
  if (*g_currentPickedRbtNode == *g_skyNode && *g_currentViewRbtNode != *g_skyNode) return;

  const double raw_dx = x - g_mouseClickX;
  const double raw_dy = g_windowHeight - y - 1 - g_mouseClickY;

  /* invert dx and/or dy depending on the situation */
  double dx_translate, dx_rotate, dy_trans, dy_rotate;
  if (*g_currentPickedRbtNode != *g_skyNode && *g_currentViewRbtNode != *g_currentPickedRbtNode) {
    /* manipulating cube, and view not from that cube */
    dx_translate = raw_dx;
    dy_trans = raw_dy;
    dx_rotate = raw_dx;
    dy_rotate = raw_dy;
  } else if (*g_currentPickedRbtNode == *g_skyNode && *g_currentViewRbtNode == *g_skyNode && g_curSkyFrame == World_Sky) {
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

  const double translateFactor = isUsingArcball() ? g_arcballScale : 0.01;

  RigTForm rigTForm;
  if (g_mouseLClickButton && !g_mouseRClickButton) {// left button down?
    rigTForm = RigTForm(Quat::makeXRotation(-dy_rotate) * Quat::makeYRotation(dx_rotate));
  } else if (g_mouseRClickButton && !g_mouseLClickButton) {// right button down?
    rigTForm = RigTForm(Cvec3(dx_translate, dy_trans, 0) * translateFactor);
  } else if (g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton)) {// middle or (left and right) button down?
    rigTForm = RigTForm(Cvec3(0, 0, -dy_trans) * translateFactor);
  }

  setFrame();

  if (g_mouseClickDown) {
    if (*g_currentPickedRbtNode == *g_skyNode && g_mouseLClickButton && !g_mouseRClickButton) {
      // Handle rotation of skyNode separately
      if (g_curSkyFrame == World_Sky) {
        // Orbit
        RigTForm g_skyRbt = g_skyNode->getRbt();
        RigTForm world_skyFrame = linFact(g_skyRbt);
        g_skyRbt = world_skyFrame * RigTForm(Quat::makeXRotation(-dy_rotate)) * inv(world_skyFrame) * g_skyRbt;
        g_skyRbt = RigTForm(Quat::makeYRotation(dx_rotate)) * g_skyRbt;
        g_skyNode->setRbt(g_skyRbt);
      } else {
        // Ego Motion
        RigTForm g_skyRbt = g_skyNode->getRbt();
        RigTForm sky_worldFrame = transFact(g_skyRbt);
        RigTForm sky_skyFrame = g_skyRbt;
        g_skyRbt = sky_skyFrame * RigTForm(Quat::makeXRotation(-dy_rotate)) * inv(sky_skyFrame) * g_skyRbt;
        g_skyRbt = sky_worldFrame * RigTForm(Quat::makeYRotation(dx_rotate)) * inv(sky_worldFrame) * g_skyRbt;
        g_skyNode->setRbt(g_skyRbt);
      }
    } else {
      g_currentPickedRbtNode->setRbt(g_Frame * rigTForm * inv(g_Frame) * g_currentPickedRbtNode->getRbt());
    }
  }

  glutPostRedisplay();// we always redraw if we changed the scene

  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1;
}


static void mouse(const int button, const int state, const int x, const int y) {
  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1;// conversion from GLUT window-coordinate-system to OpenGL window-coordinate-system

  g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
  g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
  g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

  g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
  g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
  g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

  g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;

  if (g_isPicking && g_mouseLClickButton && !g_mouseRClickButton) {
    pick();
    g_isPicking = false;
  }

  // Add this to support the snap effect
  glutPostRedisplay();
}

static void cycleView() {
  g_curViewIdx = (g_curViewIdx + 1) % g_numViews;

  switch (g_curViewIdx) {
    case 0: {
      cout << "View from sky" << endl;
      g_currentViewRbtNode = g_skyNode;
      break;
    }
    case 1: {
      cout << "View from robot 1" << endl;
      g_currentViewRbtNode = g_robot1Node;
      break;
    }
  }
}

static void cycleSkyAMatrix() {
  if (*g_currentPickedRbtNode == *g_skyNode && *g_currentViewRbtNode == *g_skyNode) {
    g_curSkyFrame = static_cast<SkyFrame>((g_curSkyFrame + 1) % 2);// toggle between World_Sky and Sky_Sky
    if (g_curSkyFrame == World_Sky) {
      cout << "Using world-sky frame" << endl;
    } else {
      cout << "Using sky-sky frame" << endl;
    }
  } else {
    cout << "Unable to change sky manipulation mode while in current view." << endl;
  }
}

// Given t in the range [0, n], perform interpolation and draw the scene
// for the particular t. Returns true if we are at the end of the animation
// sequence, or false otherwise.
static bool interpolateAndDisplay(float t) {
  static int lastFrameIdx = 1;
  int curFrameIdx = floor(t) + 1;

  if (curFrameIdx != lastFrameIdx) {
    g_script.advanceCurFrame();
    lastFrameIdx = curFrameIdx;
  }

  if (lastFrameIdx == g_script.getFrameCount() - 2) {
    lastFrameIdx = 1;
    return true;
  }

  const float alpha = t - floor(t);
  g_script.restoreInterpolatedFrame(alpha);
  glutPostRedisplay();

  return false;
}

// Interpret "ms" as milliseconds into the animation
static void animateTimerCallback(int ms) {
  float t = (float)ms/(float)g_msBetweenKeyFrames;
  bool endReached = interpolateAndDisplay(t);
  if (!endReached && g_isPlaying)
    glutTimerFunc(1000/g_animateFramesPerSecond,
                  animateTimerCallback,
                  ms + 1000/g_animateFramesPerSecond);
  else {
    g_isPlaying = false;
  }
}

static void toggleAnimation() {
  if (!g_isPlaying) {
    if (g_script.getFrameCount() < 4) {
      cerr << "[Warning] Need at least 4 keyframes, but got " << g_script.getFrameCount() << endl;
      return;
    }
    g_isPlaying = true;
    g_script.reset(1);
    animateTimerCallback(0);
  } else {
    g_isPlaying = false;
  }
}

static void keyboard(const unsigned char key, const int x, const int y) {
  switch (key) {
    case Key::Esc:
      exit(0);// ESC
    case 'h':
      cout << " ============== H E L P ==============\n\n"
           << "h\t\thelp menu\n"
           << "s\t\tsave screenshot\n"
           << "v\t\tCycle view\n"
           << "drag left mouse to rotate\n"
           << endl;
      break;
    case 's':
      glFlush();
      writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
      break;
    case 'v':
      cycleView();
      break;
    case 'm':
      cycleSkyAMatrix();
      break;
    case 'p':
      g_isPicking = true;
      break;
    case Key::Space:
      g_script.restoreCurFrame();
      break;
    case 'u':
      g_script.overwriteCurFrame();
      break;
    case '>':
      g_script.advanceCurFrame();
      break;
    case '<':
      g_script.retreatCurFrame();
      break;
    case 'd':
      g_script.deleteCurFrame();
      break;
    case 'n':
      g_script.createNewFrame();
      break;
    case 'q':
      cout << g_script << endl;
      break;
    case 'i':
      if (g_isPlaying) {
        cerr << "Unable to load script because current script is playing" << endl;
      } else {
        g_script.load(SCRIPT_PATH);
      }
      break;
    case 'w':
      g_script.save(SCRIPT_PATH);
      break;
    case 'y':
      toggleAnimation();
      break;
    case '+':
      g_msBetweenKeyFrames = max(500, g_msBetweenKeyFrames - 100);
      cout << "ms between key frames: " << g_msBetweenKeyFrames << endl;
      break;
    case '-':
      g_msBetweenKeyFrames = min(5000, g_msBetweenKeyFrames + 100);
      cout << "ms between key frames: " << g_msBetweenKeyFrames << endl;
      break;
  }
  glutPostRedisplay();
}

static void initGlutState(int argc, char *argv[]) {
  glutInit(&argc, argv);                                    // initialize Glut based on cmd-line args
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);//  RGBA pixel channels and double buffering
  glutInitWindowSize(g_windowWidth, g_windowHeight);        // create a window
  glutCreateWindow("Assignment");                         // title the window

  glutDisplayFunc(display);// display rendering callback
  glutReshapeFunc(reshape);// window reshape callback
  glutMotionFunc(motion);  // mouse movement callback
  glutMouseFunc(mouse);    // mouse click callback
  glutKeyboardFunc(keyboard);
}

static void initGLState() {
  glClearColor(128. / 255., 200. / 255., 255. / 255., 0.);
  glClearDepth(0.);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glReadBuffer(GL_BACK);
  if (!g_Gl2Compatible)
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initShadowFbo() {
  // create a texture for depth attachment
  g_depthMap.reset(new DepthTexture(g_shadowTexWidth, g_shadowTexHeight, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));

  // attach depth texture as FBO's depth buffer
  g_depthMapFbo.reset(new GlFramebuffer());
  glBindFramebuffer(GL_FRAMEBUFFER, g_depthMapFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_depthMap->getGlTexture(), 0);

  // don't check framebuffer completeness because we only have a depth attachment

  // because we don't need to write or read the depth texture
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void initMaterials() {
  // Create some prototype materials
  Material diffuse("./shaders/basic-gl2.vshader", "./shaders/diffuse-gl2.fshader");
  Material solid("./shaders/basic-gl2.vshader", "./shaders/solid-gl2.fshader");

  // copy diffuse prototype and set red color
  g_redDiffuseMat.reset(new Material(diffuse));
  g_redDiffuseMat->getUniforms().put("uColor", Cvec3f(1, 0, 0));

  // copy diffuse prototype and set blue color
  g_blueDiffuseMat.reset(new Material(diffuse));
  g_blueDiffuseMat->getUniforms().put("uColor", Cvec3f(0, 0, 1));

  // copy diffuse prototype and set green color
  g_greenDiffuseMat.reset(new Material(diffuse));
  g_greenDiffuseMat->getUniforms().put("uColor", Cvec3f(0, 1, 0));

  // copy diffuse prototype and set gray color
  g_grayDiffuseMat.reset(new Material(diffuse));
  g_grayDiffuseMat->getUniforms().put("uColor", Cvec3f(0.8, 0.8, 0.8));

  // copy solid prototype, and set to wireframed rendering
  g_arcballMat.reset(new Material(solid));
  g_arcballMat->getUniforms().put("uColor", Cvec3f(0.27f, 0.82f, 0.35f));
  g_arcballMat->getRenderStates().polygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // copy solid prototype, and set to color white
  g_lightMat.reset(new Material(solid));
  g_lightMat->getUniforms().put("uColor", Cvec3f(1, 1, 1));

  // girl material
  Material diffTex("./shaders/basic-gl2.vshader", "./shaders/diffuseTex-gl2.fshader");
  g_girlMat.reset(new Material(diffTex));
  g_girlMat->getUniforms().put("uDiffTex", shared_ptr<Texture>(new ImageTexture(TEX_FILE_PATH.c_str(), true)));

  // pick shader
  g_pickingMat.reset(new Material("./shaders/basic-gl2.vshader", "./shaders/pick-gl2.fshader"));

  // material for shadow pass
  g_shadowPassMat.reset(new Material("./shaders/basic-gl2.vshader", "./shaders/depth-gl2.fshader"));

  // material for debugging shadow
  g_debugDepthMat.reset(new Material("./shaders/debug_depth-gl2.vshader", "./shaders/debug_depth-gl2.fshader"));
}

static void initGeometry() {
  initPlane();
  initCubes();
  initSphere();
  initQuad();
}

static void constructRobot(shared_ptr<SgTransformNode> base, shared_ptr<Material> material) {

  const double ARM_LEN = 0.7,
               ARM_THICK = 0.25,
               LEG_LEN = 1.2,
               LEG_THICK = 0.25,
               TORSO_LEN = 1.5,
               TORSO_THICK = 0.25,
               TORSO_WIDTH = 1,
               HEAD_RADIUS = 0.3;

  const int NUM_JOINTS = 9,
            NUM_SHAPES = 10;

  struct JointDesc {
    int parent;
    float x, y, z;
  };

  JointDesc jointDesc[NUM_JOINTS] = {
          {-1}, // torso
          {0,  TORSO_WIDTH/2, TORSO_LEN/2 - ARM_THICK/4, 0}, // upper right arm
          {1,  ARM_LEN, 0, 0}, // lower right arm

          {0,  -TORSO_WIDTH/2, TORSO_LEN/2 - ARM_THICK/4, 0}, // upper left arm
          {3,  -ARM_LEN, 0, 0}, // lower left arm

          {0,  TORSO_WIDTH/2 - LEG_THICK/4, -TORSO_LEN/2, 0}, // upper right leg
          {5,  0, -LEG_LEN, 0}, // lower right leg

          {0,  -TORSO_WIDTH/2 + LEG_THICK/4, -TORSO_LEN/2, 0}, // upper left leg
          {7,  0, -LEG_LEN, 0}, // lower left leg
  };

  struct ShapeDesc {
    int parentJointId;
    float x, y, z, sx, sy, sz;
    shared_ptr<Geometry> geometry;
  };

  ShapeDesc shapeDesc[NUM_SHAPES] = {
          {0, 0,         0, 0, TORSO_WIDTH, TORSO_LEN, TORSO_THICK, g_cube}, // torso

          {1, ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // upper right arm
          {2, ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK*0.8, ARM_THICK*0.8, g_cube}, // lower right arm

          {3, -ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // upper left arm
          {4, -ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK*0.8, ARM_THICK*0.8, g_cube}, // lower left arm

          {5, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // upper right leg
          {6, 0, -LEG_LEN/2, 0, LEG_THICK*0.7, LEG_LEN, LEG_THICK*0.7, g_cube}, // lower right leg

          {7, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // upper left leg
          {8, 0, -LEG_LEN/2, 0, LEG_THICK*0.7, LEG_LEN, LEG_THICK*0.7, g_cube}, // lower left leg

          {0, 0, TORSO_LEN - HEAD_RADIUS, 0, HEAD_RADIUS, HEAD_RADIUS, HEAD_RADIUS, g_sphere},
  };

  shared_ptr<SgTransformNode> jointNodes[NUM_JOINTS];

  for (int i = 0; i < NUM_JOINTS; ++i) {
    if (jointDesc[i].parent == -1)
      jointNodes[i] = base;
    else {
      jointNodes[i].reset(new SgRbtNode(RigTForm(Cvec3(jointDesc[i].x, jointDesc[i].y, jointDesc[i].z))));
      jointNodes[jointDesc[i].parent]->addChild(jointNodes[i]);
    }
  }
  // The new MyShapeNode takes in a material as opposed to color
  for (int i = 0; i < NUM_SHAPES; ++i) {
    shared_ptr<SgGeometryShapeNode> shape(
            new MyShapeNode(shapeDesc[i].geometry,
                            material, // USE MATERIAL as opposed to color
                            Cvec3(shapeDesc[i].x, shapeDesc[i].y, shapeDesc[i].z),
                            Cvec3(0, 0, 0),
                            Cvec3(shapeDesc[i].sx, shapeDesc[i].sy, shapeDesc[i].sz)));
    jointNodes[shapeDesc[i].parentJointId]->addChild(shape);
  }
}

static shared_ptr<MyShapeNode> loadObj(const char* fileName, shared_ptr<Material> material) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName);

  if (!err.empty()) {
    std::cerr << "ERR: " << err << std::endl;
  }

  if (!ret) {
    std::cerr << "Failed to load/parse .obj.\n";
    assert(false);
  }

  auto vertices = vector<VertexPNX>();

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {

      VertexPNX vertex;

      vertex.p = Cvec3f(
              attrib.vertices[3 * index.vertex_index + 0],
              attrib.vertices[3 * index.vertex_index + 1],
              attrib.vertices[3 * index.vertex_index + 2]
      );

      vertex.x = Cvec2f(
              attrib.texcoords[2 * index.texcoord_index + 0],
              attrib.texcoords[2 * index.texcoord_index + 1]
      );

      vertex.n = Cvec3f(
              attrib.normals[3 * index.normal_index + 0],
              attrib.normals[3 * index.normal_index + 1],
              attrib.normals[3 * index.normal_index + 2]
      );

      // Not using indices here for simplicity
      vertices.push_back(vertex);
    }
  }

  auto geometry = make_shared<SimpleGeometryPNX>(vertices.data(), vertices.size());

  return make_shared<MyShapeNode>(geometry, material);
}

static void initScene() {
  g_world.reset(new SgRootNode());

  g_skyNode.reset(new SgRbtNode(RigTForm(Cvec3(0.0, 1.0, 25.0))));

  g_boxNode.reset(new SgRbtNode());

  // add ground
  g_boxNode->addChild(shared_ptr<MyShapeNode>(
          new MyShapeNode(g_plane, g_grayDiffuseMat,
                          Cvec3(0, g_groundY, 0),
                          Cvec3(0,0,0),
                          Cvec3(1, 1, 1))));
  // add wall
  g_boxNode->addChild(shared_ptr<MyShapeNode>(
          new MyShapeNode(g_plane, g_grayDiffuseMat,
                          Cvec3(g_planeSize, g_planeSize + g_groundY, 0),
                          Cvec3(0,0,90),
                          Cvec3(1, 1, 1))));

  g_boxNode->addChild(shared_ptr<MyShapeNode>(
          new MyShapeNode(g_plane, g_grayDiffuseMat,
                          Cvec3(0, g_planeSize + g_groundY, -g_planeSize),
                          Cvec3(90,0,0),
                          Cvec3(1, 1, 1))));

  g_lightNode.reset(new SgRbtNode(RigTForm(Cvec3(-10.0, 4.0, 10.0))));
  g_lightNode->addChild(shared_ptr<MyShapeNode>(
          new MyShapeNode(g_sphere, g_lightMat,
                          Cvec3(0,0,0),
                          Cvec3(0,0,0),
                          Cvec3(0.2, 0.2, 0.2),
                          false)
          ));

  g_robot1Node.reset(new SgRbtNode(RigTForm(Cvec3(-3, 1, 0))));

  constructRobot(g_robot1Node, g_redDiffuseMat); // a Red robot

  g_girlNode.reset(new SgRbtNode(RigTForm(Cvec3(0, 1, 0))));
  auto ptr = loadObj(OBJ_FILE_PATH.c_str(), g_girlMat);
  ptr->setAffineMatrix(Cvec3(0,-3,0),
                       Cvec3(0,0,0),
                       Cvec3(2, 2, 2));
  g_girlNode->addChild(ptr);

  g_world->addChild(g_skyNode);
  g_world->addChild(g_boxNode);
  g_world->addChild(g_lightNode);
//  g_world->addChild(g_robot1Node);
  g_world->addChild(g_girlNode);

  // initialize current picked and current view
  g_currentPickedRbtNode = g_skyNode;
  g_currentViewRbtNode = g_skyNode;
}

static void initScript() {
  g_script.initialize(g_world);
}

int main(int argc, char *argv[]) {
  try {
    initGlutState(argc, argv);

    glewInit();// load the OpenGL extensions

    cout << (g_Gl2Compatible ? "Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.3") << endl;
    if ((!g_Gl2Compatible) && !GLEW_VERSION_3_0)
      throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.3");
    else if (g_Gl2Compatible && !GLEW_VERSION_2_0)
      throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.0");

    initGLState();
    initShadowFbo();
    initMaterials();
    initGeometry();
    initScene();
    initScript();

    glutMainLoop();
    return 0;
  } catch (const runtime_error &e) {
    cout << "Exception caught: " << e.what() << endl;
    return -1;
  }
}
