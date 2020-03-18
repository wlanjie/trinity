attribute vec3 attPosition;
attribute vec2 attUV;
attribute vec4 attColor;
uniform mat4 mvpMat;

varying vec2 v_texCoord;
varying vec4 v_color;
void main()
{
    v_texCoord = attUV;
    v_color = attColor;
    gl_Position = mvpMat * vec4(attPosition, 1.0);
}