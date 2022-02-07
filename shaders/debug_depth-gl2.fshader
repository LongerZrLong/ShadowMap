varying vec2 vTexCoord;

uniform sampler2D uDepthMap;
uniform float uNearPlane;
uniform float uFarPlane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * uNearPlane * uFarPlane) / (uFarPlane + uNearPlane + z * (uFarPlane - uNearPlane));
}

void main() {
    float depthValue = texture2D(uDepthMap, vTexCoord).r;
    gl_FragColor = vec4(vec3((LinearizeDepth(depthValue) - uNearPlane) / uFarPlane), 1.0); // perspective
//    gl_FragColor = vec4(vec3(depthValue * 2.0), 1.0); // orthographic
}