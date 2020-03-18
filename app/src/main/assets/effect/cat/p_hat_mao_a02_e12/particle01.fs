precision mediump float;
uniform sampler2D diffuseMap;
varying vec2 v_texCoord;
varying vec4 v_color;

void main()
{
    gl_FragColor = texture2D(diffuseMap, v_texCoord)* v_color;
}