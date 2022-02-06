varying vec3 vPosition;
varying vec2 vTexCoord;

uniform sampler2D uScreenTexture;

void main() {
    vec4 FragColor = texture2D(uScreenTexture, vTexCoord);
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    gl_FragColor = vec4(average, average, average, 1.0);
}