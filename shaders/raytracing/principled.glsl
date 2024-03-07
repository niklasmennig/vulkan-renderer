#include "common.glsl"
#include "random.glsl"
#include "sampling.glsl"
#include "material.glsl"

float d_ggx(float nh, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float nh2 = nh * nh;
  float b = (nh2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float g1_ggx_schlick(float nv, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
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
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
} 

vec3 eval_principled(vec3 ray_in, vec3 ray_out, Material material) {
        return material.base_color * material.opacity * (1.0 - material.metallic);
}

float pdf_principled(vec3 ray_in, vec3 ray_out, Material material) {
        return 1.0;
}

BSDFSample sample_principled(vec3 ray_out, Material material, inout uint seed) {
        vec3 normal = vec3(0,1,0);
        float a = material.roughness * material.roughness;

        float pdf = 1.0;
        vec3 direction = vec3(0,1,0);
        vec3 contribution = vec3(0.0);

        if (random_float(seed) > material.opacity) {
                direction = ray_out;
                pdf = 1.0;
                contribution = vec3(1.0);
        } else {
                if (random_float(seed) < material.metallic) {
                        //metallic
                        float rnd = random_float(seed);
                        float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
                        float phi = 2.0 * PI * random_float(seed);
                        vec3 h = dir_from_thetaphi(theta, phi);

                        direction = reflect(ray_out, h);
                        pdf = 1.0 * material.metallic;
                        contribution = vec3(1.0);
                } else {
                        float pdf_difftrans = 1.0 * (1.0 - material.metallic);
                        if (random_float(seed) < material.transmission) {
                                //transmission
                                float rnd = random_float(seed);
                                float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
                                float phi = 2.0 * PI * random_float(seed);
                                vec3 h = dir_from_thetaphi(theta, phi);

                                float eta = 1.0 / material.ior;

                                direction = refract(ray_out, h, eta);
                                pdf = (1.0 * pdf_difftrans) / material.transmission;
                                contribution = vec3(1.0);
                        } else {
                                // diffuse
                                DirectionSample hemisphere_sample = sample_cosine_hemisphere(random_float(seed), random_float(seed));
                                direction = hemisphere_sample.direction;
                                pdf = (hemisphere_sample.pdf * pdf_difftrans) / (1.0 - material.transmission);
                                contribution = material.base_color;
                        }
                }
        }
        
        BSDFSample result;
        result.pdf = pdf;
        result.direction = direction;
        result.contribution = contribution;
        return result;
}