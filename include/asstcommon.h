#ifndef ASSTCOMMON_H
#define ASSTCOMMON_H

#include <cstddef>
#include <vector>
#include <memory>
#include <stdexcept>

#include "glsupport.h"
#include "uniforms.h"
#include "material.h"

extern const bool g_Gl2Compatible;

extern std::shared_ptr<Material> g_overridingMaterial;

// takes MVM and its normal matrix to the shaders
inline void sendModelViewNormalMatrix(Uniforms& uniforms, const Matrix4& MVM, const Matrix4& NMVM) {
  uniforms.put("uModelViewMatrix", MVM).put("uNormalMatrix", NMVM);
}

// takes a projection matrix and send to the the shaders
inline void sendProjectionMatrix(Uniforms &uniforms, const Matrix4 &projMatrix) {
  uniforms.put("uProjMatrix", projMatrix);
}

#endif