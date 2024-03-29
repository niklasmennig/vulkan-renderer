#include "common.glsl"
#include "sampling.glsl"

// BRDF Formulations: http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

//https://www.mathematik.uni-marburg.de/~thormae/lectures/graphics2/graphics_4_3_ger_web.html#1
vec3 fresnel_schlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
} 

float d_ggx(float NoH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NoH2 = NoH * NoH;
  float b = (NoH2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float g1_ggx_schlick(float NdotV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float g_smith(float NoV, float NoL, float roughness) {
  float g1_l = g1_ggx_schlick(NoL, roughness);
  float g1_v = g1_ggx_schlick(NoV, roughness);
  return g1_l * g1_v;
}

vec3 eval_ggx(vec3 ray_in, vec3 ray_out, vec3 base_color, float opacity, float metallic, float fresnel_reflect, float roughness, float transmission, float ior) {
  vec3 normal = vec3(0,1,0);

  float no = dot(normal, ray_out);
  float ni = dot(normal, ray_in);

  if (no * ni > 0) { // non-transmissive
    no = abs(no);
    ni = abs(ni);

    vec3 h = normalize(ray_in + ray_out);
    float nh = clamp(dot(normal, h), 0.0, 1.0);
    float oh = clamp(dot(ray_out, h), 0.0, 1.0);

    vec3 f0 = vec3(0.16 * (fresnel_reflect * fresnel_reflect));
    f0 = mix(f0, base_color, metallic);

    // glossy
    vec3 f = fresnel_schlick(oh, f0);
    float d = d_ggx(nh, roughness);
    float g = g_smith(no, ni, roughness);
    vec3 spec = (d * g * f) / max(4.0 * no * ni, 0.001);

    // diffuse
    vec3 rho = vec3(1.0) - f;
    rho *= (1.0 - metallic) * (1.0 - transmission);
    vec3 diff = rho * base_color / PI;

    return (diff+spec) * opacity;
  } else { // transmissive
    float eta = 1.0 / ior;
    vec3 h = normalize(ray_in + ray_out * eta);

  }
}

float pdf_ggx(vec3 ray_in, vec3 ray_out, vec3 base_color, float opacity, float metallic, float fresnel_reflect, float roughness, float transmission, float ior) {
  vec3 normal = vec3(0,1,0);
  vec3 h = normalize(ray_in + ray_out);

  float no = clamp(dot(normal, ray_out), 0.0, 1.0);
  float ni = clamp(dot(normal, ray_in), 0.0, 1.0);
  float nh = clamp(dot(normal, h), 0.0, 1.0);
  float oh = clamp(dot(ray_out, h), 0.0, 1.0);

  float d = d_ggx(nh, roughness);
  float g = g_smith(no, ni, roughness);

  float theta = dot(ray_out, normal);

  float pdf_diffuse = cos(theta) * sin(theta) / PI;
  float pdf_specular = d * cos(theta) * sin(theta);

  return (pdf_diffuse + pdf_specular) / 2.0;
}

BSDFSample sample_ggx(in vec3 out_dir, 
              in vec3 baseColor, float opacity, in float metallicness, 
              in float fresnelReflect, in float roughness, in float transmission, in float ior, in uint seed, bool front_facing) 
{
  vec3 normal = vec3(0,1,0);

  // handle opacity
  if (random_float(seed) > opacity) {
    BSDFSample res;
    res.contribution = vec3(1.0);
    res.direction = -out_dir;
    res.pdf = 1.0;
    return res;
  }

  if(random_float(seed) < 0.5) { // non-specular light
    if(random_float(seed) * 2.0 < transmission) { // transmitted light
           
      float a = roughness * roughness;
      float rnd = random_float(seed);
      float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
      // float theta = 0.01;
      float phi = 2.0 * PI * random_float(seed);
      
      vec3 h_local = dir_from_thetaphi(theta, phi);

      // compute L from sampled H
      float eta = 1.0 / ior;
      // normal.y *= -1;
      vec3 l_local = refract(out_dir, h_local, eta);
      if (!front_facing) l_local = refract(out_dir, -h_local, eta);
      
      // all required dot products
      float NoV = clamp(dot(normal, out_dir), 0.0, 1.0);
      float NoL = abs(dot(normal, l_local));
      float NoH = abs(dot(normal, h_local));
      float VoH = clamp(dot(out_dir, h_local), 0.0, 1.0);     
      
      // F0 for dielectics in range [0.0, 0.16] 
      // default FO is (0.16 * 0.5^2) = 0.04
      vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
      // in case of metals, baseColor contains F0
      f0 = mix(f0, baseColor, metallicness);
    
      vec3 F = fresnel_schlick(VoH, f0);
      float D = d_ggx(NoH, roughness);
      float G = g_smith(NoV, NoL, roughness);
      vec3 contrib = vec3(1.0);
    
      // contrib *= 2.0; // compensate for splitting diffuse and specular
      
      BSDFSample res;
      res.contribution = contrib;
      res.direction = l_local;
      // res.pdf = D * sin(theta) * cos(theta);
      res.pdf = 1.0 / transmission;
      return res;
      
    } else { // diffuse light
      
      // sampled indirect diffuse direction in normal space
      DirectionSample diffuse_sample = sample_cosine_hemisphere(random_float(seed), random_float(seed));
      vec3 l_local = diffuse_sample.direction;

      float theta = thetaphi_from_dir(l_local).x;
      
       // half vector
      vec3 h_local = normalize(out_dir + l_local);
      float VoH = clamp(dot(out_dir, h_local), 0.0, 1.0);     
      
      // F0 for dielectics in range [0.0, 0.16] 
      // default FO is (0.16 * 0.5^2) = 0.04
      vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
      // in case of metals, baseColor contains F0
      f0 = mix(f0, baseColor, metallicness);    
      vec3 F = fresnel_schlick(VoH, f0);
      
      vec3 notSpec = vec3(1.0) - F; // if not specular, use as diffuse
      notSpec *= (1.0 - metallicness); // no diffuse for metals
    
      vec3 contrib = notSpec * baseColor;
      
      BSDFSample res;
      res.contribution = contrib;
      res.direction = l_local;
      res.pdf = diffuse_sample.pdf / 2.0;
      return res;
    }
  } else {// specular light

    float a = roughness * roughness;
    float rnd = random_float(seed);
    float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
    float phi = 2.0 * PI * random_float(seed);
    vec3 h_local = dir_from_thetaphi(theta, phi);

    vec3 l_local = reflect(-out_dir, h_local);

    // all required dot products
    float NoV = clamp(dot(normal, out_dir), 0.0, 1.0);
    float NoL = clamp(dot(normal, l_local), 0.0, 1.0);
    float NoH = clamp(dot(normal, h_local), 0.0, 1.0);
    float VoH = clamp(dot(out_dir, h_local), 0.0, 1.0);     
    
    // F0 for dielectics in range [0.0, 0.16] 
    // default FO is (0.16 * 0.5^2) = 0.04
    vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
    // in case of metals, baseColor contains F0
    f0 = mix(f0, baseColor, metallicness);
  
    // specular microfacet (cook-torrance) BRDF
    vec3 F = fresnel_schlick(VoH, f0);
    float D = d_ggx(NoH, roughness);
    float G = g_smith(NoV, NoL, roughness);
    vec3 contrib =  F * G * VoH / max((NoH * NoV), 0.001);
    
    BSDFSample res;
    res.contribution = contrib;
    res.direction = l_local;
    res.pdf = h_local.y / 2.0;
    return res;
  } 
}