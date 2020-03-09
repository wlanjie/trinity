//
//  face.h
//  opengl
//
//  Created by wlanjie on 2020/3/2.
//  Copyright © 2020 com.wlanjie.opengl. All rights reserved.
//

#ifndef face_h
#define face_h

#include <stdio.h>
#include "frame_buffer.h"

#include "opengl.h"

#if __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "android_xlog.h"
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#define LOGE(format, ...) fprintf(stdout, format, __VA_ARGS__)
#endif

#include "face_detection.h"

static const char* FACE_POINT_VERTEX_SHADER =
        "attribute vec4 position;"
        "void main() {"
        "   gl_Position = position;"
        "   gl_PointSize = 45.0;"
        "}";

static const char* FACE_POINT_FRAGMENT_SHADER =
        "void main() {"
        "   gl_FragColor = vec4(0.0,1.0,0.0,1.0);"
        "}";
        
static const char* FACE_EXTRA_POINT_VERTEX_SHADER =
        "attribute vec4 position;"
        "void main() {"
        "   gl_Position = position;"
        "   gl_PointSize = 45.0;"
        "}";

static const char* FACE_EXTRA_POINT_FRAGMENT_SHADER =
        "void main() {"
        "   gl_FragColor = vec4(1.0,0.0,0.0,1.0);"
        "}";        
        

const char* FACE_MARKUP_VERTEX_SHADER =
    "attribute vec3 position;"
    "attribute vec2 inputTextureCoordinate;"
    "varying vec2 textureCoordinate;"
    "varying vec2 textureCoordinate2;"
    "void main() {"
    "   gl_Position = vec4(position, 1.);"
    "   textureCoordinate = inputTextureCoordinate;"
    "   textureCoordinate2 = position.xy * 0.5 + 0.5;"
    "}";

const char* FACE_MARKUP_FRAGMENT_SHADER =
    "varying vec2 textureCoordinate;"
    "varying vec2 textureCoordinate2;"
    "uniform sampler2D inputImageTexture;"
    "uniform sampler2D inputImageTexture2;"
    "uniform float intensity;"
    "uniform int blendMode;"
    "float blendHardLight(float base, float blend) {"
    "    return blend < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend));"
    "}"
    "vec3 blendHardLight(vec3 base, vec3 blend) {"
    "    return vec3(blendHardLight(base.r,blend.r),blendHardLight(base.g,blend.g),blendHardLight(base.b,blend.b));"
    "}"

    "float blendSoftLight(float base, float blend) {"
    "    return (blend<0.5)?(base+(2.0*blend-1.0)*(base-base*base)):(base+(2.0*blend-1.0)*(sqrt(base)-base));"
    "}"
    "vec3 blendSoftLight(vec3 base, vec3 blend) {"
    "    return vec3(blendSoftLight(base.r,blend.r),blendSoftLight(base.g,blend.g),blendSoftLight(base.b,blend.b));"
    "}"
    "vec3 blendMultiply(vec3 base, vec3 blend) {"
    "    return base*blend;"
    "}"
    "float blendOverlay(float base, float blend) {"
    "    return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));"
    "}"
    "vec3 blendOverlay(vec3 base, vec3 blend) {"
    "    return vec3(blendOverlay(base.r,blend.r),blendOverlay(base.g,blend.g),blendOverlay(base.b,blend.b));"
    "}"
    "vec3 blendFunc(vec3 base, vec3 blend, int blendMode) {"
    "    if (blendMode == 0) {"
    "        return blend;"
    "    } else if (blendMode == 15) {"
    "       return blendMultiply(base, blend);"
    "    } else if (blendMode == 17) {"
    "        return blendOverlay(base, blend);"
    "    } else if (blendMode == 22) {"
    "        return blendHardLight(base, blend);"
    "    }"
    "    return blend;"
    "}"
    "void main() {"
    "    vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);"
    "    fgColor = fgColor * intensity;"
    "    vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);"
    "    if (fgColor.a == 0.0) {"
    "        gl_FragColor = bgColor;"
    "        return;"
    "    }"
    "    vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), blendMode);"
    "    gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);"
//    "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
//    "   gl_FragColor = fgColor;"
    "}";
        

static float land_mark[] = {
    146.69012,
    176.36653,
    147.43333,
    186.29279,
    148.6127,
    196.25972,
    150.38898,
    206.1813,
    152.48157,
    216.00528,
    154.84981,
    225.8128,
    157.70853,
    235.48413,
    161.40039,
    244.83386,
    166.22977,
    253.73709,
    172.2808,
    262.0086,
    179.17354,
    269.65305,
    186.71468,
    276.91913,
    194.64233,
    283.68005,
    203.04936,
    289.8985,
    212.3331,
    295.011,
    222.6648,
    298.11322,
    233.45834,
    298.4821,
    243.34111,
    295.99762,
    251.78839,
    290.81552,
    258.59277,
    283.92175,
    264.51624,
    276.31494,
    270.20312,
    268.6408,
    275.75867,
    260.76782,
    280.8286,
    252.74976,
    285.2124,
    244.36981,
    288.6377,
    235.64056,
    291.3692,
    226.63326,
    293.6554,
    217.45056,
    295.30817,
    208.15834,
    296.2428,
    198.8099,
    296.58636,
    189.4002,
    296.40643,
    180.05132,
    295.93112,
    170.8013,
    162.21164,
    167.41162,
    174.49937,
    159.45103,
    187.7869,
    158.17184,
    201.10698,
    158.83946,
    213.57437,
    161.36235,
    246.06778,
    160.61205,
    256.79514,
    157.1607,
    268.29514,
    155.44969,
    279.71582,
    155.61958,
    289.98227,
    162.22589,
    231.19962,
    180.30101,
    232.2302,
    194.12614,
    233.28546,
    208.23964,
    234.33209,
    222.23563,
    215.19405,
    230.45853,
    226.27249,
    232.43417,
    233.95502,
    234.07236,
    241.27452,
    232.01434,
    250.18289,
    228.55719,
    176.76874,
    181.91612,
    184.9711,
    176.10786,
    203.85977,
    177.34744,
    209.7261,
    184.60504,
    201.41104,
    186.7325,
    184.24933,
    185.87143,
    249.4902,
    182.7622,
    254.61044,
    175.16074,
    272.17493,
    172.2864,
    279.57568,
    177.39516,
    273.39532,
    181.90451,
    257.52173,
    184.12375,
    175.19987,
    166.37537,
    188.13887,
    165.99306,
    201.10573,
    166.80548,
    213.66776,
    168.38591,
    246.4972,
    167.32684,
    257.51013,
    164.85648,
    268.68765,
    163.07877,
    279.74506,
    162.4202,
    194.67284,
    174.71817,
    192.77933,
    187.43085,
    193.52979,
    181.54399,
    263.0868,
    171.80194,
    265.70676,
    184.11147,
    264.54874,
    178.3244,
    219.97232,
    182.86105,
    240.83542,
    181.94067,
    215.22543,
    213.65591,
    248.09625,
    212.21088,
    210.35754,
    223.56622,
    253.65015,
    221.41225,
    197.6759,
    246.77386,
    210.75452,
    245.61713,
    224.03568,
    244.59415,
    233.00378,
    245.66895,
    241.37799,
    243.92227,
    252.1563,
    243.70355,
    262.1295,
    244.0937,
    255.46628,
    257.98907,
    243.60008,
    267.56747,
    233.54153,
    269.89276,
    222.72769,
    268.85553,
    207.3868,
    260.72592,
    203.29584,
    248.47023,
    223.71603,
    249.7411,
    233.09692,
    250.49094,
    241.89365,
    248.96794,
    256.6396,
    246.08023,
    242.3882,
    257.39618,
    233.13391,
    259.26904,
    223.208,
    258.3466,
    194.07779,
    180.45633,
    263.18317,
    177.4411,
};

static GLuint faceIndexs[] = {
        // 左眉毛  10个三角形
        33, 34, 64,
        64, 34, 65,
        65, 34, 107,
        107, 34, 35,
        35, 36, 107,
        107, 36, 66,
        66, 107, 65,
        66, 36, 67,
        67, 36, 37,
        37, 67, 43,
        // 右眉毛 10个三角形
        43, 38, 68,
        68, 38, 39,
        39, 68, 69,
        39, 40, 108,
        39, 108, 69,
        69, 108, 70,
        70, 108, 41,
        41, 108, 40,
        41, 70, 71,
        71, 41, 42,
        // 左眼 21个三角形
        0, 33, 52,
        33, 52, 64,
        52, 64, 53,
        64, 53, 65,
        65, 53, 72,
        65, 72, 66,
        66, 72, 54,
        66, 54, 67,
        54, 67, 55,
        67, 55, 78,
        67, 78, 43,
        52, 53, 57,
        53, 72, 74,
        53, 74, 57,
        74, 57, 73,
        72, 54, 104,
        72, 104, 74,
        74, 104, 73,
        73, 104, 56,
        104, 56, 54,
        54, 56, 55,
        // 右眼 21个三角形
        68, 43, 79,
        68, 79, 58,
        68, 58, 59,
        68, 59, 69,
        69, 59, 75,
        69, 75, 70,
        70, 75, 60,
        70, 60, 71,
        71, 60, 61,
        71, 61, 42,
        42, 61, 32,
        61, 60, 62,
        60, 75, 77,
        60, 77, 62,
        77, 62, 76,
        75, 77, 105,
        77, 105, 76,
        105, 76, 63,
        105, 63, 59,
        105, 59, 75,
        59, 63, 58,
        // 左脸颊 16个
        0, 52, 1,
        1, 52, 2,
        2, 52, 57,
        2, 57, 3,
        3, 57, 4,
        4, 57, 109,
        57, 109, 74,
        74, 109, 56,
        56, 109, 80,
        80, 109, 82,
        82, 109, 7,
        7, 109, 6,
        6, 109, 5,
        5, 109, 4,
        56, 80, 55,
        55, 80, 78,
        // 右脸颊 16个
        32, 61, 31,
        31, 61, 30,
        30, 61, 62,
        30, 62, 29,
        29, 62, 28,
        28, 62, 110,
        62, 110, 76,
        76, 110, 63,
        63, 110, 81,
        81, 110, 83,
        83, 110, 25,
        25, 110, 26,
        26, 110, 27,
        27, 110, 28,
        63, 81, 58,
        58, 81, 79,
        // 鼻子部分 16个
        78, 43, 44,
        43, 44, 79,
        78, 44, 80,
        79, 81, 44,
        80, 44, 45,
        44, 81, 45,
        80, 45, 46,
        45, 81, 46,
        80, 46, 82,
        81, 46, 83,
        82, 46, 47,
        47, 46, 48,
        48, 46, 49,
        49, 46, 50,
        50, 46, 51,
        51, 46, 83,
        // 鼻子和嘴巴中间三角形 14个
        7, 82, 84,
        82, 84, 47,
        84, 47, 85,
        85, 47, 48,
        48, 85, 86,
        86, 48, 49,
        49, 86, 87,
        49, 87, 88,
        88, 49, 50,
        88, 50, 89,
        89, 50, 51,
        89, 51, 90,
        51, 90, 83,
        83, 90, 25,
        // 上嘴唇部分 10个
        84, 85, 96,
        96, 85, 97,
        97, 85, 86,
        86, 97, 98,
        86, 98, 87,
        87, 98, 88,
        88, 98, 99,
        88, 99, 89,
        89, 99, 100,
        89, 100, 90,
        // 下嘴唇部分 10个
        90, 100, 91,
        100, 91, 101,
        101, 91, 92,
        101, 92, 102,
        102, 92, 93,
        102, 93, 94,
        102, 94, 103,
        103, 94, 95,
        103, 95, 96,
        96, 95, 84,
        // 唇间部分 8个
        96, 97, 103,
        97, 103, 106,
        97, 106, 98,
        106, 103, 102,
        106, 102, 101,
        106, 101, 99,
        106, 98, 99,
        99, 101, 100,
        // 嘴巴与下巴之间的部分(关键点7 到25 与嘴巴鼻翼围起来的区域) 24个
        7, 84, 8,
        8, 84, 9,
        9, 84, 10,
        10, 84, 95,
        10, 95, 11,
        11, 95, 12,
        12, 95, 94,
        12, 94, 13,
        13, 94, 14,
        14, 94, 93,
        14, 93, 15,
        15, 93, 16,
        16, 93, 17,
        17, 93, 18,
        18, 93, 92,
        18, 92, 19,
        19, 92, 20,
        20, 92, 91,
        20, 91, 21,
        21, 91, 22,
        22, 91, 90,
        22, 90, 23,
        23, 90, 24,
        24, 90, 25
};

static float face_texture_coordinate[] = {
    0.302451, 0.384169,
    0.302986, 0.409377,
    0.304336, 0.434977,
    0.306984, 0.460683,
    0.311010, 0.486447,
    0.316537, 0.511947,
    0.323069, 0.536942,
    0.331312, 0.561627,
    0.342011, 0.585088,
    0.355477, 0.607217,
    0.371142, 0.627774,
    0.388459, 0.646991,
    0.407041, 0.665229,
    0.426325, 0.682694,
    0.447468, 0.697492,
    0.471782, 0.707060,
    0.500000, 0.709867,
    0.528218, 0.707060,
    0.552532, 0.697492,
    0.573675, 0.682694,
    0.592959, 0.665229,
    0.611541, 0.646991,
    0.628858, 0.627774,
    0.644523, 0.607217,
    0.657989, 0.585088,
    0.668688, 0.561627,
    0.676931, 0.536942,
    0.683463, 0.511947,
    0.688990, 0.486447,
    0.693016, 0.460683,
    0.695664, 0.434977,
    0.697014, 0.409377,
    0.697549, 0.384169,
    0.331655, 0.354725,
    0.354609, 0.331785,
    0.387080, 0.325436,
    0.420446, 0.330125,
    0.452685, 0.339996,
    0.547315, 0.339996,
    0.579554, 0.330125,
    0.612920, 0.325436,
    0.645391, 0.331785,
    0.668345, 0.354725,
    0.500000, 0.405156,
    0.500000, 0.442322,
    0.500000, 0.480116,
    0.500000, 0.517378,
    0.457729, 0.542442,
    0.476911, 0.546376,
    0.500000, 0.550557,
    0.523089, 0.546376,
    0.542271, 0.542442,
    0.366597, 0.404028,
    0.385132, 0.392425,
    0.428177, 0.397495,
    0.442446, 0.414082,
    0.422818, 0.419177,
    0.382917, 0.415929,
    0.557554, 0.414082,
    0.571823, 0.397495,
    0.614868, 0.392425,
    0.633403, 0.404028,
    0.617083, 0.415929,
    0.577182, 0.419177,
    0.360880, 0.349748,
    0.391440, 0.348304,
    0.421788, 0.352051,
    0.451601, 0.358026,
    0.548399, 0.358026,
    0.578212, 0.352051,
    0.608560, 0.348304,
    0.639120, 0.349748,
    0.407165, 0.390906,
    0.402591, 0.420584,
    0.406113, 0.405280,
    0.592835, 0.390906,
    0.597409, 0.420584,
    0.593887, 0.405280,
    0.471223, 0.409619,
    0.528777, 0.409619,
    0.455607, 0.495169,
    0.544393, 0.495169,
    0.441855, 0.523363,
    0.558145, 0.523363,
    0.426186, 0.593516,
    0.453348, 0.586128,
    0.481258, 0.582594,
    0.500000, 0.584476,
    0.518742, 0.582594,
    0.546652, 0.586128,
    0.573814, 0.593516,
    0.556544, 0.620391,
    0.531320, 0.639672,
    0.500000, 0.644911,
    0.468680, 0.639672,
    0.443456, 0.620391,
    0.433718, 0.595595,
    0.466898, 0.597025,
    0.500000, 0.599883,
    0.533102, 0.597025,
    0.566282, 0.595595,
    0.534634, 0.610720,
    0.500000, 0.616173,
    0.465366, 0.610720,
    0.406113, 0.405280,
    0.593887, 0.405280,
    0.366597, 0.404028,
    0.371848, 0.398748,
    0.378649, 0.394894,
    0.386219, 0.392124,
    0.394413, 0.390794,
    0.402774, 0.390598,
    0.411165, 0.391436,
    0.419295, 0.393357,
    0.426892, 0.396722,
    0.433465, 0.401488,
    0.438854, 0.407474,
    0.442446, 0.414082,
    0.436384, 0.416284,
    0.429069, 0.417953,
    0.421545, 0.419403,
    0.413920, 0.420469,
    0.406041, 0.420698,
    0.398066, 0.420187,
    0.390427, 0.418642,
    0.383246, 0.416078,
    0.376779, 0.412565,
    0.370994, 0.408368,
    0.633403, 0.404028,
    0.628152, 0.398748,
    0.621351, 0.394894,
    0.613781, 0.392124,
    0.605587, 0.390794,
    0.597226, 0.390598,
    0.588835, 0.391436,
    0.580705, 0.393357,
    0.573108, 0.396722,
    0.566535, 0.401488,
    0.561146, 0.407474,
    0.557554, 0.414082,
    0.563616, 0.416284,
    0.570930, 0.417953,
    0.578455, 0.419403,
    0.586080, 0.420469,
    0.593959, 0.420698,
    0.601934, 0.420187,
    0.609573, 0.418642,
    0.616754, 0.416078,
    0.623221, 0.412565,
    0.629006, 0.408368,
    0.331655, 0.354725,
    0.345423, 0.338137,
    0.364839, 0.327427,
    0.387080, 0.325436,
    0.409419, 0.327950,
    0.431440, 0.332797,
    0.452685, 0.339996,
    0.350925, 0.351659,
    0.371041, 0.348509,
    0.391440, 0.348304,
    0.411721, 0.350365,
    0.431851, 0.353879,
    0.451601, 0.358026,
    0.668345, 0.354725,
    0.654577, 0.338137,
    0.635161, 0.327427,
    0.612920, 0.325436,
    0.590581, 0.327950,
    0.568560, 0.332797,
    0.547315, 0.339996,
    0.649075, 0.351659,
    0.628959, 0.348509,
    0.608560, 0.348304,
    0.588279, 0.350365,
    0.568149, 0.353879,
    0.548399, 0.358026,
    0.434929, 0.590082,
    0.444183, 0.588272,
    0.453349, 0.586127,
    0.462555, 0.584150,
    0.471859, 0.582687,
    0.481258, 0.582594,
    0.490612, 0.583760,
    0.500000, 0.584476,
    0.509388, 0.583760,
    0.518742, 0.582594,
    0.528141, 0.582687,
    0.537445, 0.584150,
    0.546651, 0.586127,
    0.555817, 0.588272,
    0.565071, 0.590082,
    0.442022, 0.595481,
    0.450315, 0.596007,
    0.458611, 0.596431,
    0.466897, 0.597025,
    0.475169, 0.597793,
    0.483427, 0.598703,
    0.491692, 0.599501,
    0.500000, 0.599747,
    0.508308, 0.599501,
    0.516573, 0.598703,
    0.524831, 0.597793,
    0.533103, 0.597025,
    0.541389, 0.596431,
    0.549685, 0.596007,
    0.557978, 0.595481,
    0.441074, 0.600438,
    0.448886, 0.604514,
    0.457009, 0.607918,
    0.465364, 0.610720,
    0.473828, 0.613157,
    0.482453, 0.614958,
    0.491189, 0.616066,
    0.500000, 0.616367,
    0.508811, 0.616066,
    0.517547, 0.614958,
    0.526172, 0.613157,
    0.534636, 0.610720,
    0.542991, 0.607918,
    0.551114, 0.604514,
    0.558926, 0.600438,
    0.431864, 0.604088,
    0.438539, 0.614066,
    0.446099, 0.623383,
    0.454936, 0.631506,
    0.465044, 0.637970,
    0.476219, 0.642352,
    0.487989, 0.644682,
    0.500000, 0.645199,
    0.512011, 0.644682,
    0.523781, 0.642352,
    0.534956, 0.637970,
    0.545064, 0.631506,
    0.553901, 0.623383,
    0.561461, 0.614066,
    0.568136, 0.604088,
    0.426186, 0.593516,
    0.573814, 0.593516,
    0.433718, 0.595595,
    0.566282, 0.595595,
};

#define WIDTH 512
#define HEIGHT 512

static GLfloat vertex_point[] = {
    0.141461849,
    0.0721842051,
    0.160995007
};

static GLfloat triangles_texture[] = {
//    1.35692751,
    0.0,
    0.456855685,
    0.0
//    1.35692751
};

namespace trinity {

class FacePoint {
 public:
    FacePoint()
        : source_width_(0)
        , source_height_(0)
        , target_width_(0)
        , target_height_(0) {
        buffer_length_ = 111 * 2 * sizeof(float);
        face_point_buffer_ = new float[buffer_length_];
        program_ = CreateProgram(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
        prop_program_ = CreateProgram(FACE_MARKUP_VERTEX_SHADER, FACE_MARKUP_FRAGMENT_SHADER);
        extra_point_program_ = CreateProgram(FACE_EXTRA_POINT_VERTEX_SHADER, FACE_EXTRA_POINT_FRAGMENT_SHADER);
        point_program_ = CreateProgram(FACE_POINT_VERTEX_SHADER, FACE_POINT_FRAGMENT_SHADER);
        point_position_location_ = glGetAttribLocation(point_program_, "position");
        
        glGenBuffers(1, &vertex_buffer_);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER, buffer_length_, &vertex_buffer_, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glGenFramebuffers(1, &frameBuffer_id_);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
        
        glActiveTexture(GL_TEXTURE1);
        glGenTextures(1, &texture_id_);
        glBindTexture(GL_TEXTURE_2D, texture_id_);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);

        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
    #if __ANDROID__
            LOGE("frame buffer error");
    #endif
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        vertex_coordinate_ = new GLfloat[8];
        texture_coordinate_ = new GLfloat[8];
        vertex_coordinate_[0] = -1.0F;
        vertex_coordinate_[1] = -1.0F;
        vertex_coordinate_[2] = 1.0F;
        vertex_coordinate_[3] = -1.0F;
        vertex_coordinate_[4] = -1.0F;
        vertex_coordinate_[5] = 1.0F;
        vertex_coordinate_[6] = 1.0F;
        vertex_coordinate_[7] = 1.0F;

        texture_coordinate_[0] = 0.0F;
        texture_coordinate_[1] = 0.0F;
        texture_coordinate_[2] = 1.0F;
        texture_coordinate_[3] = 0.0F;
        texture_coordinate_[4] = 0.0F;
        texture_coordinate_[5] = 1.0F;
        texture_coordinate_[6] = 1.0F;
        texture_coordinate_[7] = 1.0F;
        
        glGenBuffers(1, &vertex_buffer_);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER, buffer_length_, &vertex_buffer_, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    ~FacePoint() {
        if (point_program_ != 0) {
            glDeleteProgram(point_program_);
            point_program_ = 0;
        }
        delete[] face_point_buffer_;
    }

    void SetSourceSize(int width, int height) {
        source_width_ = width;
        source_height_ = height;
    }

    void SetTargetSize(int width, int height) {
        target_width_ = width;
        target_height_ = height;
    }

    float ConvertX(float x, int width) {
        float centerX = width / 2.0f;
        float t = x - centerX;
        return t / centerX;
    }

    float ConvertY(float y, int height) {
        float centerY = height / 2.0f;
        float s = centerY - y;
        return s / centerY;
    }
    
    template<class T>
     int GetLength(const T & arr) {
         return sizeof(arr) / sizeof(arr[0]);
     }
     
    GLfloat* Vertex() {
        static float extra_land_mark[10];
        // 嘴唇中心
        extra_land_mark[0] = (land_mark[98 * 2] + land_mark[102 * 2]) / 2;
        extra_land_mark[1] = (land_mark[98 * 2 + 1] + land_mark[102 * 2 + 1]) / 2;

        // 左眉毛中心
        extra_land_mark[2] = (land_mark[35 * 2] + land_mark[65 * 2]) / 2;
        extra_land_mark[3] = (land_mark[35 * 2 + 1] + land_mark[65 * 2 + 1]) / 2;

        // 右眉毛中心
        extra_land_mark[4] = (land_mark[40 * 2] + land_mark[70 * 2]) / 2;
        extra_land_mark[5] = (land_mark[40 * 2 + 1] + land_mark[70 * 2 + 1]) / 2;
        
        // 左脸中心
        extra_land_mark[6] = (land_mark[4 * 2] + land_mark[82 * 2]) / 2;
        extra_land_mark[7] = (land_mark[4 * 2 + 1] + land_mark[82 * 2 + 1]) / 2;

        // 右脸中心
        extra_land_mark[8] = (land_mark[28 * 2] + land_mark[83 * 2]) / 2;
        extra_land_mark[9] = (land_mark[28 * 2 + 1] + land_mark[83 * 2 + 1]) / 2;
        
        static GLfloat vertex[111 * 2];
        for (int i = 0; i < 106; i++) {
            float x = land_mark[i * 2] / source_width_;
            float y = 1.0F - land_mark[i * 2 + 1] / source_height_;
            vertex[i * 2] = x;
            vertex[i * 2 + 1] = y;
//            vertex[i * 2] = x * 2 - 1;
//            vertex[i * 2 + 1] = y * 2 - 1;
        }
        
        static float extra_vertex[10];
        for (int i = 0; i < 5; i++) {
            float x = extra_land_mark[i * 2] / source_width_;
            float y = 1.0F - extra_land_mark[i * 2 + 1] / source_height_;
            vertex[212 + i * 2] = x;
            vertex[212 + i * 2 + 1] = y;
//            vertex[212 + i * 2] = x * 2 - 1;
//            vertex[212 + i * 2 + 1] = y * 2 - 1;
//            extra_vertex[i * 2] = x * 2 - 1;
//            extra_vertex[i * 2 + 1] = y * 2 - 1;
        }
        
        for (int i = 0; i < 111; i++) {
            float x = vertex[i * 2];
            float y = vertex[i * 2 + 1];
            
            vertex[i * 2] = x * 2 - 1;
            vertex[i * 2 + 1] = y * 2 - 1;
        }
        return vertex;
    }
    
    GLfloat* TextureCoordinate() {
        static GLfloat textureCoordinate[111 * 2];
        int point_count = GetLength(faceTextureCoordinates) / 2;
        for (int i = 0; i < 111; i++) {
            float x = (faceTextureCoordinates[i * 2 + 0] * 1280 - 712.0F) / 161.6F;
            float y = (faceTextureCoordinates[i * 2 + 1] * 1280 - 536.0F) / 80.0F;
//            x = x < 0 ? 0 : x;
//            y = y < 0 ? 0 : y;
//            textureCoordinate[i * 2] = x > 1 ? 1 : x;
//            textureCoordinate[i * 2 + 1] = 1.0F - (y > 1 ? 1 : y);

            textureCoordinate[i * 2] = x;
            textureCoordinate[i * 2 + 1] = y;
        }
        return textureCoordinate;
    }

    GLuint OnDrawFrame(GLuint texture_id, GLuint prop_texture_id) {
        GLfloat* vertex = Vertex();
        GLfloat* textureCoordinate = TextureCoordinate();
        int element_count = sizeof(faceIndexs) / sizeof(GLuint);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_id_);
        
        glViewport(0, 0, 512, 512);
        glClearColor(1.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prop_program_);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        GLint input_image_texture_location = glGetUniformLocation(prop_program_, "inputImageTexture");
        glUniform1i(input_image_texture_location, 2);
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, prop_texture_id);
        GLint input_image_texture_location2 = glGetUniformLocation(prop_program_, "inputImageTexture2");
        glUniform1i(input_image_texture_location2, 3);

        GLint blend_model_location = glGetUniformLocation(prop_program_, "blendMode");
        glUniform1i(blend_model_location, 15);
        GLint intensity_location = glGetUniformLocation(prop_program_, "intensity");
        glUniform1f(intensity_location, 1.0F);
        auto prop_position = static_cast<GLuint>(glGetAttribLocation(prop_program_, "position"));
        glEnableVertexAttribArray(prop_position);
        glVertexAttribPointer(prop_position, 2, GL_FLOAT, GL_FALSE, 0, vertex);
        
        auto prop_input_texture_coordinate = static_cast<GLuint>(glGetAttribLocation(prop_program_,
                                                                                  "inputTextureCoordinate"));
        glEnableVertexAttribArray(prop_input_texture_coordinate);
        glVertexAttribPointer(prop_input_texture_coordinate, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinate);
        glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, faceIndexs);
        glDisableVertexAttribArray(prop_position);
        glDisableVertexAttribArray(prop_input_texture_coordinate);
        
//        glUseProgram(program_);
//        glEnable(GL_BLEND);
//        glDepthMask(false);
////        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
////        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
//        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
//        glActiveTexture(GL_TEXTURE2);
//        glBindTexture(GL_TEXTURE_2D, texture_id);
//        auto input_image_texture = static_cast<GLuint>(glGetUniformLocation(program_, "inputImageTexture"));
//        glUniform1i(input_image_texture, 2);
//        auto position = static_cast<GLuint>(glGetAttribLocation(program_, "position"));
//        glEnableVertexAttribArray(position);
//        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertex_coordinate_);
//        auto input_texture_coordinate = static_cast<GLuint>(glGetAttribLocation(program_, "inputTextureCoordinate"));
//        glEnableVertexAttribArray(input_texture_coordinate);
//        glVertexAttribPointer(input_texture_coordinate, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texture_coordinate_);
//        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//        glDisableVertexAttribArray(position);
//        glDisableVertexAttribArray(input_texture_coordinate);
//        glDisable(GL_BLEND);
        
        glUseProgram(point_program_);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_length_, vertex);
        glEnableVertexAttribArray(point_position_location_);
        glVertexAttribPointer(point_position_location_, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glPointSize(5);
        glDrawArrays(GL_POINTS, 0, 111);
        
//        glUseProgram(extra_point_program_);
//        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
//        glBufferSubData(GL_ARRAY_BUFFER, 0, 10 * 2 * sizeof(float), extra_vertex);
//        glEnableVertexAttribArray(point_position_location_);
//        glVertexAttribPointer(point_position_location_, 2, GL_FLOAT, GL_FALSE, 0, 0);
//        glBindBuffer(GL_ARRAY_BUFFER, 0);
//        glPointSize(5);
//        GLuint extra_position = glGetAttribLocation(extra_point_program_, "position");
//        glEnableVertexAttribArray(extra_position);
//        glVertexAttribPointer(extra_position, 2, GL_FLOAT, GL_FALSE, 0, vertex_point);
//        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return texture_id_;
    }

 private:
    GLuint CreateProgram(const char* vertex, const char* fragment) {
        GLuint program = glCreateProgram();
        auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
        auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        CompileShader(vertex, vertexShader);
        CompileShader(fragment, fragmentShader);
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        Link(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return program;
    }

    void CompileShader(const char* shader_string, GLuint shader) {
        glShaderSource(shader, 1, &shader_string, nullptr);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        printf("%s\n", shader_string);
        printf("==================\n");
        if (!compiled) {
            GLint infoLen;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                auto *buf = reinterpret_cast<char*>(malloc((size_t) infoLen));
                if (buf) {
                    LOGE("shader_string: %s", shader_string);
                    glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                    LOGE("Could not compile %d:\n%s\n", shader, buf);
                    free(buf);
                }
                glDeleteShader(shader);
            }
        }
    }

    void Link(GLuint program) {
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus == GL_FALSE) {
            GLint infoLen;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                auto *buf = reinterpret_cast<char*>(malloc((size_t) infoLen));
                if (buf) {
                    glGetProgramInfoLog(program, infoLen, nullptr, buf);
                    printf("%s", buf);
                    free(buf);
                }
                glDeleteProgram(program);
                program = 0;
            }
        }
    }

 private:
    GLuint program_;
    GLuint extra_point_program_;
    GLuint point_program_;
    GLuint prop_program_;
    GLint point_position_location_;
    GLuint vertex_buffer_;
    int buffer_length_;
    float* face_point_buffer_;
    int source_width_;
    int source_height_;
    int target_width_;
    int target_height_;
    GLuint frameBuffer_id_;
    GLuint texture_id_;
    GLfloat* vertex_coordinate_;
    GLfloat* texture_coordinate_;
    GLuint prop_texture_id_;
};

}

#endif /* face_h */
