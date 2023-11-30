#ifndef COLOR_GLSL
#define COLOR_GLSL

// adapted from https://github.com/PearCoding/Ignis/blob/master/src/artic/core/color.art

vec3 rgb_to_xyz(vec3 rgb) {
    return vec3(
        0.4124564 * rgb.r + 0.3575761 * rgb.g + 0.1804375 * rgb.b,
        0.2126729 * rgb.r + 0.7151522 * rgb.g + 0.0721750 * rgb.b,
        0.0193339 * rgb.r + 0.1191920 * rgb.g + 0.9503041 * rgb.b
    );
}

vec3 xyz_to_rgb(vec3 xyz) {
    return vec3(
        3.2404542 * xyz.r - 1.5371385 * xyz.g - 0.4985314 * xyz.b,
        -0.9692660 * xyz.r + 1.8760108 * xyz.g + 0.0415560 * xyz.b,
        0.0556434 * xyz.r - 0.2040259 * xyz.g + 1.0572252 * xyz.b
    );
}

vec3 rgb_to_xyY(vec3 rgb) {
    vec3 xyz = rgb_to_xyz(rgb);
    float n = xyz.r + xyz.g + xyz.b;
    return n <= 1.0e-10 ? vec3(0.0) : vec3(xyz.r / n, xyz.g / n, xyz.g);
}

vec3 xyY_to_rgb(vec3 xyY) {
    return xyY.g <= 1.0e-10 ? vec3(0.0) : xyz_to_rgb(vec3(xyY.r * xyY.b / xyY.g, xyY.b, (1.0 - xyY.r - xyY.g) * xyY.b / xyY.g));
}

vec3 rgb_to_hsv(vec3 rgb) {
    vec4 p = rgb.g < rgb.b ? vec4(rgb.b, rgb.g, -1, 2.0 / 3.0) : vec4(rgb.g, rgb.b, 0, -1.0 / 3.0);
    vec4 q = rgb.r < p.x ? vec4(p.x, p.y, p.w, rgb.r) : vec4(rgb.r, p.y, p.z, p.x);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;

    return vec3(
        abs(q.z + (q.w - q.y) / (6 * d + e)),
        d / (q.x + e),
        q.x
    );
}

vec3 hsv_to_rgb(vec3 hsv) {
    vec3 p = abs(vec3(fract(hsv.r + 1), fract(hsv.r + 2.0 / 3.0), fract(hsv.r + 1.0 / 3.0)) * 6 - vec3(3.0));

    return vec3(
        hsv.b * mix(1, clamp(p.x - 1, 0, 1), hsv.g),
        hsv.b * mix(1, clamp(p.y - 1, 0, 1), hsv.g),
        hsv.b * mix(1, clamp(p.z - 1, 0, 1), hsv.g)
    );
}

vec3 rgb_to_hsl(vec3 rgb) {
    vec3 hsv = rgb_to_hsv(rgb);
    float l = hsv.g * (1.0 - hsv.b / 2.0);

    return vec3(
        hsv.r,
        l,
        (hsv.g - l) / min(l, 1.0 - l)
    );
}

vec3 hsl_to_rgb(vec3 hsl) {
    float v = hsl.b + hsl.g * min(hsl.b, 1.0 - hsl.b);
    vec3 hsv = vec3 (
        hsl.r,
        v,
        2.0 * (v - hsl.b) / v
    );

    return hsv_to_rgb(hsv);
}

#endif