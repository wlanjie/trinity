#ifdef GL_ES
precision highp float;
varying highp vec2 textureCoordinate;
varying highp float aspectRatio;
#else
varying vec2 textureCoordinate;
varying float aspectRatio;
#endif
uniform sampler2D inputImageTexture;
uniform float facePoints[106 * 2];
uniform float bigEyeScale;

// 眼睛圆内放大
vec2 enlargeEye(vec2 textureCoordinate, vec2 originPosition, float radius, float delta) {
    float weight = distance(vec2(textureCoordinate.x, textureCoordinate.y / aspectRatio),
        vec2(originPosition.x, originPosition.y / aspectRatio)) / radius;
    weight = 1.0 - (1.0 - weight * weight) * delta;
    weight = clamp(weight, 0.0, 1.0);
    textureCoordinate = originPosition + (textureCoordinate - originPosition) * weight;
    return textureCoordinate;
}

vec2 bigEyeFunc(vec2 currentCoordinate) {
    vec2 faceIndexs[2];
    faceIndexs[0] = vec2(74., 72.);
    faceIndexs[1] = vec2(77., 75.);
    for (int i = 0; i < 2; i++) {
        int originIndex = int(faceIndexs[i].x);
        int targetIndex = int(faceIndexs[i].y);
        vec2 originPoint = vec2(facePoints[originIndex * 2], facePoints[originIndex * 2 + 1]);
        vec2 targetPoint = vec2(facePoints[targetIndex * 2], facePoints[targetIndex * 2 + 1]);
        float radius = distance(vec2(targetPoint.x, targetPoint. y / aspectRatio),
            vec2(originPoint.x, originPoint.y / aspectRatio));
        radius = radius * 5.;
        currentCoordinate = enlargeEye(currentCoordinate, originPoint, radius, bigEyeScale);    
    }
    return currentCoordinate;
}

void main() {
    vec2 positionToUse = bigEyeFunc(textureCoordinate);
    gl_FragColor = texture2D(inputImageTexture, positionToUse);
}
