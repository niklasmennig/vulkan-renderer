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
        vec3 normal = vec3(0,1,0);

        float no = dot(normal, ray_out);
        float ni = dot(normal, ray_in);

        if (no >= 0 && ni >= 0) { // non-transmissive
                no = abs(no);
                ni = abs(ni);

                vec3 h = normalize(ray_in + ray_out);
                float nh = clamp(dot(normal, h), 0.0, 1.0);
                float oh = clamp(dot(ray_out, h), 0.0, 1.0);

                vec3 f0 = vec3(0.16 * (material.fresnel * material.fresnel));
                f0 = mix(f0, material.base_color, material.metallic);

                // glossy
                vec3 f = fresnel_schlick(oh, f0);
                float d = d_ggx(nh, material.roughness);
                float g = g_smith(no, ni, material.roughness);
                vec3 spec = (d * g * f) / max(4.0 * no * ni, 0.001);

                // diffuse
                vec3 rho = vec3(1.0) - f;
                rho *= (1.0 - material.metallic) * (1.0 - material.transmission);
                vec3 diff = rho * material.base_color / PI;

                return (diff+spec) * material.opacity;
        } else { // transmissive
                float eta = 1.0 / material.ior;
                vec3 h = normalize(ray_in + ray_out * eta);
                return vec3(0.0);
        }
}

float pdf_principled(vec3 ray_in, vec3 ray_out, Material material) {
        vec3 normal = vec3(0,1,0);
        vec3 h = normalize(ray_in + ray_out);

        float no = clamp(dot(normal, ray_out), 0.0, 1.0);
        float ni = clamp(dot(normal, ray_in), 0.0, 1.0);
        float nh = clamp(dot(normal, h), 0.0, 1.0);
        float oh = clamp(dot(ray_out, h), 0.0, 1.0);

        float d = d_ggx(nh, material.roughness);
        float g = g_smith(no, ni, material.roughness);

        float theta = dot(ray_out, normal);

        float pdf_diffuse = cos(theta) * sin(theta) / PI;
        float pdf_specular = d * cos(theta) * sin(theta);

        return (pdf_diffuse + pdf_specular) / 2.0;
}

BSDFSample sample_metallic(vec3 ray_out, Material material, inout uint seed) {
        float a = material.roughness * material.roughness;
        float rnd = random_float(seed);
        float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
        float phi = 2.0 * PI * random_float(seed);
        vec3 h = dir_from_thetaphi(theta, phi);

        BSDFSample result;
        result.direction = reflect(ray_out, h);
        result.pdf = FLT_MAX;
        result.weight = vec3(1.0);
        return result;
}

BSDFSample sample_transmissive(vec3 ray_out, Material material, inout uint seed) {
        float a = material.roughness * material.roughness;
        float rnd = random_float(seed);
        float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
        float phi = 2.0 * PI * random_float(seed);
        vec3 h = dir_from_thetaphi(theta, phi);

        float eta = 1.0 / material.ior;

        BSDFSample result;
        result.direction = refract(ray_out, h, eta);
        result.pdf = FLT_MAX;
        result.weight = vec3(1.0);
        return result;
}

BSDFSample sample_diffuse(vec3 ray_out, Material material, inout uint seed) {
        DirectionSample hemisphere_sample = sample_cosine_hemisphere(random_float(seed), random_float(seed));

        BSDFSample result;
        result.direction = hemisphere_sample.direction;
        result.pdf = hemisphere_sample.pdf;
        result.weight = material.base_color;
        return result;
}

BSDFSample sample_principled(vec3 ray_out, Material material, inout uint seed) {
        vec3 normal = vec3(0,1,0);

        BSDFSample lobe_sample;
        float lobe_pdf = 0.0;

        if (random_float(seed) > material.opacity) {
                //non-opaque
                lobe_sample.direction = ray_out;
                lobe_sample.pdf = FLT_MAX;
                lobe_sample.weight = vec3(1.0);
                lobe_pdf = 1.0 - material.opacity;
        } else {
                if (random_float(seed) < material.metallic) {
                        //metallic
                        lobe_sample = sample_metallic(ray_out, material, seed);
                        lobe_pdf = (material.opacity) * material.metallic;             
                } else {
                        float pdf_difftrans = (material.opacity) * (1.0 - material.metallic);
                        if (random_float(seed) < material.transmission) {
                                //transmission
                                lobe_sample = sample_transmissive(ray_out, material, seed);
                                lobe_pdf = pdf_difftrans * material.transmission;
                        } else {
                                // diffuse
                                lobe_sample = sample_diffuse(ray_out, material, seed);
                                lobe_pdf = pdf_difftrans * (1.0 - material.transmission);
                        }
                }
        }
        
        BSDFSample result;
        result.pdf = lobe_sample.pdf * lobe_pdf;
        result.direction = lobe_sample.direction;
        result.weight = lobe_sample.weight;
        return result;
}