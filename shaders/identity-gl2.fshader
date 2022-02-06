varying vec3 vPosition;
varying vec2 vTexCoord;

uniform sampler2D uScreenTexture;

void main() {
    gl_FragColor = texture2D(uScreenTexture, vTexCoord);
}