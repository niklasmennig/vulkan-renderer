#include "../common.glsl"
#include "../random.glsl"
#include "sampling.glsl"
#include "material.glsl"
#include "fresnel.glsl"

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

vec3 eval_principled(vec3 ray_dir, vec3 light_dir, Material material) {
        bool is_transmission = ray_dir.y * light_dir.y < 0;
        float eta = 1.0 / material.ior;
        if (ray_dir.y < 0.0) eta = material.ior;

        vec3 n = vec3(0,1,0);
        vec3 h = is_transmission ? normalize(ray_dir + light_dir * eta) : normalize(ray_dir + light_dir);

        float nv = clamp(ray_dir.y, 0.0, 1.0);
        float nl = clamp(light_dir.y, 0.0, 1.0);
        float nh = clamp(h.y, 0.0, 1.0);
        float vh = clamp(dot(ray_dir, h), 0.0, 1.0);

        
        FresnelTerm f = fresnel(eta, abs(ray_dir.y));
        float d = d_ggx(nh, material.roughness);
        float g = g_smith(nv, nl, material.roughness);


        if (is_transmission) {
                // specular
                vec3 spec = material.base_color * (d * g * (1.0 - f.factor)) / max(4.0 * nv * nl, 0.001);

                // return spec;
                return vec3(0);
        } else {
                // specular
                vec3 spec = material.base_color * (d * g * f.factor) / max(4.0 * nv * nl, 0.001);

                // diffuse
                float nspec = 1.0 - f.factor;
                nspec *= (1.0 - material.metallic) * (1.0 - material.transmission);
                vec3 diff = nspec * material.base_color / PI;

                // return diff + spec;
                return diff;
        }
}

float pdf_principled(vec3 ray_dir, vec3 light_dir, Material material) {
        vec3 n = vec3(0,1,0);
        vec3 h = normalize(ray_dir + light_dir);
        
        float nv = clamp(ray_dir.y, 0.0, 1.0);
        float nl = clamp(light_dir.y, 0.0, 1.0);
        float nh = clamp(h.y, 0.0, 1.0);
        float vh = clamp(dot(ray_dir, h), 0.0, 1.0);

        vec3 f0 = vec3(0.16 * (material.fresnel * material.fresnel));
        f0 = mix(f0, material.base_color, material.metallic);

        vec3 f = fresnel_schlick(vh, f0);
        float d = d_ggx(nh, material.roughness);
        float g = g_smith(nv, nl, material.roughness);

        float theta_l = acos(light_dir.y);
        float theta_h = acos(h.y);

        bool is_transmission = ray_dir.y * light_dir.y < 0;

        if (is_transmission) {
                return d * cos(theta_h) * sin(theta_h);
        } else {
                float pdf_diff = 1.0 / PI;
                float pdf_spec = d * cos(theta_h) * sin(theta_h);

                return pdf_diff + pdf_spec;
        }
}

BSDFSample sample_principled(vec3 ray_out, Material material, inout uint seed) {
        vec3 normal = vec3(0,1,0);

        BSDFSample lobe_sample;
        lobe_sample.weight = vec3(0.0);
        lobe_sample.pdf = 1.0;

        if (random_float(seed) < 0.5) {
                // non-specular
                if (random_float(seed) * 2.0 < material.transmission) {
                        // transmission
                        float eta = 1.0 / material.ior;

                        vec3 n = vec3(0,1,0);
                        float front_facing = dot(ray_out, n);

                        if (front_facing < 0.0) {
                                n *= -1;
                                eta = material.ior;
                        }

                        float a = material.roughness * material.roughness;
                        float rnd = random_float(seed);
                        float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
                        float phi = 2.0 * PI * random_float(seed);

                        vec3 h = dir_from_thetaphi(theta, phi) * n.y;
                        vec3 ray_in = refract(-ray_out, h, eta);

                        float nv = clamp(dot(n, ray_out), 0.0, 1.0);
                        float nl = clamp(dot(-n, ray_in), 0.0, 1.0);
                        float nh = clamp(dot(n, h), 0.0, 1.0);
                        float vh = clamp(dot(ray_out, h), 0.0, 1.0);

                        vec3 f0 = vec3(0.16 * material.fresnel * material.fresnel);
                        f0 = mix(f0, material.base_color, material.metallic);
                        vec3 f = fresnel_schlick(vh, f0);

                        float d = d_ggx(nh, material.roughness);
                        float g = g_smith(nv, nl, material.roughness);

                        float pdf = d * cos(theta) * sin(theta);
                        lobe_sample.weight = vec3(0);// (material.base_color * (vec3(1.0) - f) * g * vh / max(nh * nv, 0.001)) * 2.0;
                        lobe_sample.direction = ray_in;
                        lobe_sample.pdf = pdf;
                } else {
                        // diffuse
                        float theta = asin(sqrt(random_float(seed)));
                        float phi = 2.0 * PI * random_float(seed);

                        vec3 ray_in = dir_from_thetaphi(theta, phi);

                        vec3 h = normalize(ray_out + ray_in);
                        float vh = clamp(dot(ray_in, h), 0.0, 1.0);

                        vec3 f0 = vec3(0.16 * material.fresnel * material.fresnel);
                        f0 = mix(f0, material.base_color, material.metallic);
                        vec3 f = fresnel_schlick(vh, f0);

                        vec3 nspec = vec3(1.0) - f;
                        nspec *= (1.0 - material.metallic);

                        float pdf = cos(theta) * sin(theta) / PI;
                        lobe_sample.weight = (nspec * material.base_color) * 2.0;
                        lobe_sample.direction = ray_in;
                        lobe_sample.pdf = pdf;
                }
        } else {
                // specular
                float a = material.roughness * material.roughness;
                float rnd = random_float(seed);
                float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
                float phi = 2.0 * PI * random_float(seed);

                vec3 h = dir_from_thetaphi(theta, phi);
                vec3 ray_in = reflect(-ray_out, h);

                float nv = clamp(ray_out.y, 0.0, 1.0);
                float nl = clamp(ray_in.y, 0.0, 1.0);
                float nh = clamp(h.y, 0.0, 1.0);
                float vh = clamp(dot(ray_out, h), 0.0, 1.0);

                vec3 f0 = vec3(0.16 * material.fresnel * material.fresnel);
                f0 = mix(f0, material.base_color, material.metallic);
                vec3 f = fresnel_schlick(vh, f0);

                float d = d_ggx(nh, material.roughness);
                float g = g_smith(nv, nl, material.roughness);

                float pdf = d * cos(theta) * sin(theta);
                lobe_sample.weight = (f * g * vh / max(nh * nv, 0.001)) * 0.0;
                lobe_sample.direction = ray_in;
                lobe_sample.pdf = pdf;
        }
        
        BSDFSample result;
        result.pdf = lobe_sample.pdf;
        result.direction = lobe_sample.direction;
        result.weight = lobe_sample.weight;
        return result;
}