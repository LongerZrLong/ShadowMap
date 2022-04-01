uniform vec3 uLight;

uniform sampler2D uDiffTex;
uniform sampler2D uDepthMap;

varying vec3 vNormal;
varying vec3 vPosition;
varying vec4 vFragPosLightProj;
varying vec2 vTexCoord;

#define EPS 1e-3

void main() {
    vec3 fragPosLightNdc = vFragPosLightProj.xyz / vFragPosLightProj.w; // [-1, 1] ^ 3
    vec3 fragPosLightNdcShift = fragPosLightNdc * 0.5 + 0.5;  // [0, 1] ^ 3

    float occDepth = texture2D(uDepthMap, fragPosLightNdcShift.xy).r;
    float realDepth = fragPosLightNdcShift.z;

    float factor = 1.0;
    if (occDepth > realDepth + EPS) {
        // occluded
        factor *= 0.5;
    }

    vec3 tolight = normalize(uLight - vPosition);
    vec3 normal = normalize(vNormal);

    vec3 diffColor = texture2D(uDiffTex, vTexCoord).xyz;
    vec3 ambient = 0.2 * diffColor;
    vec3 diffuse = diffColor * max(0.0, dot(normal, tolight));
    vec3 intensity = ambient + diffuse;

    gl_FragColor = factor * vec4(intensity, 1.0);
}
