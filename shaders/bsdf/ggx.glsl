#ifndef GGX_GLSL
#define GGX_GLSL

// inline float evaluateGGX(float alpha, const Vector &wh) {
//     float nDotH = Frame::cosTheta(wh);
//     float a     = Frame::cosPhiSinTheta(wh) / alpha;
//     float b     = Frame::sinPhiSinTheta(wh) / alpha;
//     float c     = sqr(a) + sqr(b) + sqr(nDotH);
//     return 1 / (Pi * sqr(alpha * c));
// }

float d_ggx(vec3 h, float roughness) {
  float nh = h.y;
  float a     = h.x / roughness;
  float b     = h.z / roughness;
  float c     = a*a + b*b + nh*nh;
  return 1.0 / (PI * (roughness * c) * (roughness * c));

}

// inline float smithG1(float alpha, const Vector &wh, const Vector &w) {
//     /// Ensure correct orientation by projecting both @c w and @c wh into the
//     /// upper hemisphere and checking that the angle they form is less than 90°
//     if (w.dot(wh) * Frame::cosTheta(w) * Frame::cosTheta(wh) <= 0)
//         return 0;

//     /// Special case: if @c cosTheta of @c w is large, we know that the tangens
//     /// will be @c 0 and hence our result is @c 1
//     if (Frame::absCosTheta(w) >= 1)
//         return 1;

//     const float a2tanTheta2 = sqr(alpha) * Frame::tanTheta2(w);
//     return 2 / (1 + sqrt(1 + a2tanTheta2));
// }

float g_smith(vec3 v, vec3 h, float roughness) {
  if (dot(v, h) * v.y * h.y <= 0.0) return 0;

  float cos_2 = v.y * v.y;
  float tan_theta2 = (1.0 - cos_2) / cos_2;
  float a2_tan_theta2 = (roughness * roughness) * tan_theta2;
  return 2.0 / (1.0 + sqrt(1.0 + a2_tan_theta2));
}

// inline Vector sampleGGXVNDF(float alpha, const Vector &wo, const Point2 &rnd) {
//     // Addition: flip sign of incident vector for transmission
//     float sgn = copysign(1, Frame::cosTheta(wo));
//     // Section 3.2: transforming the view direction to the hemisphere
//     // configuration
//     Vector Vh =
//         sgn * Vector(alpha * wo.x(), alpha * wo.y(), wo.z()).normalized();
//     // Section 4.1: orthonormal basis (with special case if cross product is
//     // zero)
//     float lensq = Vh.x() * Vh.x() + Vh.y() * Vh.y();
//     Vector T1 =
//         lensq > 0 ? Vector(-Vh.y(), Vh.x(), 0) / sqrt(lensq) : Vector(1, 0, 0);
//     Vector T2 = Vh.cross(T1);
//     // Section 4.2: parameterization of the projected area
//     float r   = sqrt(rnd.x());
//     float phi = 2 * Pi * rnd.y();
//     float t1  = r * cos(phi);
//     float t2  = r * sin(phi);
//     float s   = 0.5f * (1 + Vh.z());
//     t2        = (1 - s) * sqrt(1 - sqr(t1)) + s * t2;
//     // Section 4.3: reprojection onto hemisphere
//     Vector Nh = t1 * T1 + t2 * T2 + safe_sqrt(1 - sqr(t1) - sqr(t2)) * Vh;
//     // Section 3.4: transforming the normal back to the ellipsoid configuration
//     Vector Ne =
//         Vector(alpha * Nh.x(), alpha * Nh.y(), max(0.f, Nh.z())).normalized();
//     return sgn * Ne;
// }

vec3 sample_ggx(vec3 v, float roughness, inout uint seed) {
  float sgn = sign(v.y);
  vec3 h = sgn * normalize(vec3(roughness * v.x, v.y, roughness * v.z));
  float lensqr = h.x * h.x + h.y * h.y;
  vec3 T1 = lensqr > 0 ? vec3(-h.z, h.x, 0) / sqrt(lensqr) : vec3(1,0,0);
  vec3 T2 = cross(h, T1);
  
  float r = sqrt(random_float(seed));
  float phi = 2.0 * PI * random_float(seed);

  float t1 = r * cos(phi);
  float t2 = r * sin(phi);
  float s = 0.5 * (1.0 + h.y);
  t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

  float sqr = 1.0 - t1 * t1 - t2 * t2;
  float safe_sqrt = sqr <= 0 ? 0 : sqrt(sqr);
  vec3 nh = t1 * T1 + t2 * T2 + safe_sqrt * h;
  vec3 ne = normalize(vec3(roughness * nh.x, max(0.0, nh.y), roughness * nh.z));

  return sgn * ne;
}

// inline float pdfGGXVNDF(float alpha, const Vector &wh, const Vector &wo) {
//     // clang-format off
//     return microfacet::evaluateGGX(alpha, wh) *
//            microfacet::smithG1(alpha, wh, wo) *
//            abs(wh.dot(wo)) / Frame::absCosTheta(wo);
//     // clang-format on
// }

float pdf_ggx(vec3 v, vec3 h, float roughness) {
  return d_ggx(h, roughness) * g_smith(v, h, roughness) * dot(v, h) / abs(v.y);
}

// inline float detReflection(const Vector &normal, const Vector &wo) {
//     return 1 / abs(4 * normal.dot(wo));
// }

float det_reflection(vec3 v, vec3 h) {
  return 1.0 / abs(4.0 * dot(h, v));
}

// inline float detRefraction(const Vector &normal, const Vector &wi,
//                            const Vector &wo, float eta) {
//     return sqr(eta) *
//            abs(normal.dot(wi) / sqr(normal.dot(wi) * eta + normal.dot(wo)));
// }

float det_refraction(vec3 ray_in, vec3 ray_out, vec3 h, float eta) {
  float d = dot(h, ray_in) * eta + dot(h, ray_out);
  return (eta * eta) * abs(dot(h, ray_in) / (d * d));
}

#endif