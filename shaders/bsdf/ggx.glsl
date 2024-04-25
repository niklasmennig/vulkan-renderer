#ifndef GGX_GLSL
#define GGX_GLSL

float d_ggx(float nh, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float nh2 = nh * nh;
  float b = (nh2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float g1_ggx_schlick(float nv, float roughness) {
  float r = 0.5 + 0.5 * roughness;
  float k = (r * r) / 2.0;
  float denom = nv * (1.0 - k) + k;
  return nv / denom;
}

float g_smith(float nv, float nl, float roughness) {
  float g1_l = g1_ggx_schlick(nl, roughness);
  float g1_v = g1_ggx_schlick(nv, roughness);
  return g1_l * g1_v;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
  return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

#endif