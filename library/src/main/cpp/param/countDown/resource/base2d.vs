attribute vec3 position;
attribute vec2 inputTextureCoordinate;
uniform float scaling;
uniform float angle;
uniform int display_width;
uniform int display_height;

varying vec2 textureCoordinate;

void main() {
    vec3 posToUse = position;
    posToUse.xy = posToUse.xy * (1.0 + clamp(scaling, 0.0, 1.0));
    float angleToUse = clamp(angle, 0.0, 1.0);
    posToUse.xy *= vec2(float(display_width/2), float(display_height/2));
    mat3 rot = mat3(cos(angleToUse), -sin(angleToUse), 0.0, sin(angleToUse), cos(angleToUse), 0.0, 0.0, 0.0, 1.0);
    posToUse = posToUse * rot;
    posToUse.xy /= vec2(float(display_width/2), float(display_height/2));
    gl_Position = vec4(posToUse, 1.0);
    textureCoordinate = inputTextureCoordinate;
}
