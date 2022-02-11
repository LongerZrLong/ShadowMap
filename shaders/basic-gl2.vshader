uniform mat4 uProjMatrix;
uniform mat4 uModelViewMatrix;
uniform mat4 uNormalMatrix;

uniform mat4 uEyeLightMatrix;
uniform mat4 uLightProjMatrix;

attribute vec3 aPosition;
attribute vec3 aNormal;

varying vec3 vNormal;
varying vec3 vPosition;
varying vec4 vFragPosLightProj;

void main() {
  vNormal = vec3(uNormalMatrix * vec4(aNormal, 0.0));

  // send position (eye coordinates) to fragment shader
  vec4 tPosition = uModelViewMatrix * vec4(aPosition, 1.0);
  vPosition = vec3(tPosition);
  gl_Position = uProjMatrix * tPosition;

  vFragPosLightProj = uLightProjMatrix * uEyeLightMatrix * tPosition;
}