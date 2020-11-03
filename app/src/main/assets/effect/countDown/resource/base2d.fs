#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D inputImageTexture;
uniform int rotate_blur;
uniform int display_width1;
uniform int display_height1;

varying vec2 textureCoordinate;

void main() {
    // if(rotate_blur == 1) {
    //     vec2 center  = vec2 (0.5 * float(display_width1), 0.5 * float(display_height1));
    //     vec2 screenCoord = textureCoordinate.xy * vec2(display_width1, display_height1);
    //     float r = length(screenCoord - center);
    //     float ang = atan((screenCoord - center).y, (screenCoord - center).x);

    //     vec4 pix = vec4(0.0);
    //     int pixNum = 0;
    //     float yy, xx;
    //     float rot = 0.0;

    //     for (float i = 0.0; i < 10.0; i = i + 0.1){
    //         rot -= 0.001;
    //         yy = floor(abs(r - i)*sin(ang + rot)) + center.y;
    //         xx = floor(abs(r + 0.1)*cos(ang + rot)) + center.x;
    //         if(yy >= 1.0 && yy <= float(display_height1) && xx >= 1.0 && xx <= float(display_width1)){
    //             pix += texture2D(inputImageTexture, vec2(xx, yy)/vec2(display_width1, display_height1));
    //             pixNum += 1;
    //         }
    //     }

    //     pix = pix/float(pixNum);
    //     gl_FragColor = pix;
    // }else{
        gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
    // }
    
}
