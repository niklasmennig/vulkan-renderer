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

vec3 eval_ggx(vec3 ray_in, vec3 ray_out, mat3 tbn, vec3 base_color, float opacity, float metallic, float fresnel_reflect, float roughness, float transmission, float ior) {
  vec3 normal = tbn[2];
  vec3 h = normalize(ray_in + ray_out);

  float no = clamp(dot(normal, ray_out), 0.0, 1.0);
  float ni = clamp(dot(normal, ray_in), 0.0, 1.0);
  float nh = clamp(dot(normal, h), 0.0, 1.0);
  float oh = clamp(dot(ray_out, h), 0.0, 1.0);

  vec3 f0 = vec3(0.16 * (fresnel_reflect * fresnel_reflect));
  f0 = mix(f0, base_color, metallic);

  // specular
  vec3 f = fresnel_schlick(oh, f0);
  float d = d_ggx(nh, roughness);
  float g = g_smith(no, ni, roughness);
  vec3 spec = (d * g * f) / max(4.0 * no * ni, 0.001);

  // diffuse
  vec3 rho = vec3(1.0) - f;
  rho *= (1.0 - metallic) * (1.0 - transmission);
  vec3 diff = rho * base_color / PI;

  return (diff + spec) * opacity;
}

float pdf_ggx(vec3 ray_in, vec3 ray_out, mat3 tbn, vec3 base_color, float opacity, float metallic, float fresnel_reflect, float roughness, float transmission, float ior) {
  vec3 normal = tbn[2];
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

BSDFSample sample_ggx(in vec3 V, in mat3 tbn, 
              in vec3 baseColor, float opacity, in float metallicness, 
              in float fresnelReflect, in float roughness, in float transmission, in float ior, in vec4 random) 
{
    mat3 tbn_local = tbn;
    vec3 N = tbn[2];

    // handle opacity
    if (random.w > opacity) {
      BSDFSample res;
      res.contribution = vec3(1.0);
      res.direction = -V;
      res.pdf = 1.0;
      res.specular = true;
      return res;
    }

    if(random.z < 0.5) { // non-specular light
    if((2.0 * random.z) < transmission) { // transmitted light
      vec3 forwardNormal = N;
      float frontFacing = dot(V, N);
      float eta = 1.0 / ior;
      if(frontFacing < 0.0) {
         forwardNormal = -N;
         tbn_local[2] = forwardNormal;
         eta = ior;
      } 
      
      // important sample GGX
      // pdf = D * cos(theta) * sin(theta)
      float a = roughness * roughness;
      float theta = acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
      float phi = 2.0 * PI * random.x;
      
      vec3 localH = dir_from_thetaphi(theta, phi);
      vec3 H = tbn_local * localH;  
      
      // compute L from sampled H
      vec3 L = refract(-V, H, eta);
      
      // all required dot products
      float NoV = clamp(dot(forwardNormal, V), 0.0, 1.0);
      float NoL = clamp(dot(-forwardNormal, L), 0.0, 1.0); // reverse normal
      float NoH = clamp(dot(forwardNormal, H), 0.0, 1.0);
      float VoH = clamp(dot(V, H), 0.0, 1.0);     
      
      // F0 for dielectics in range [0.0, 0.16] 
      // default FO is (0.16 * 0.5^2) = 0.04
      vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
      // in case of metals, baseColor contains F0
      f0 = mix(f0, baseColor, metallicness);
    
      vec3 F = fresnel_schlick(VoH, f0);
      float D = d_ggx(NoH, roughness);
      float G = g_smith(NoV, NoL, roughness);
      vec3 contrib = baseColor * (vec3(1.0) - F) * G * VoH / max((NoH * NoV), 0.001);
    
      contrib *= 2.0; // compensate for splitting diffuse and specular
      
      BSDFSample res;
      res.contribution = contrib;
      res.direction = normalize(L);
      res.pdf = 1.0;
      res.specular = true;
      return res;
      
    } else { // diffuse light
      
      // important sampling diffuse
      // pdf = cos(theta) * sin(theta) / PI
      // sampled indirect diffuse direction in normal space
      DirectionSample diffuse_sample = sample_cosine_hemisphere(random.x, random.y);
      vec3 localDiffuseDir = diffuse_sample.direction;
      vec3 L = tbn_local * localDiffuseDir;

      float theta = acos(localDiffuseDir.z);
      
       // half vector
      vec3 H = normalize(V + L);
      float VoH = clamp(dot(V, H), 0.0, 1.0);     
      
      // F0 for dielectics in range [0.0, 0.16] 
      // default FO is (0.16 * 0.5^2) = 0.04
      vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
      // in case of metals, baseColor contains F0
      f0 = mix(f0, baseColor, metallicness);    
      vec3 F = fresnel_schlick(VoH, f0);
      
      vec3 notSpec = vec3(1.0) - F; // if not specular, use as diffuse
      notSpec *= (1.0 - metallicness); // no diffuse for metals
    
      vec3 contrib = notSpec * baseColor;
      contrib *= 2.0; // compensate for splitting diffuse and specular
      
      BSDFSample res;
      res.contribution = contrib;
      res.direction = normalize(L);
      res.pdf = diffuse_sample.pdf;
      res.specular = false;
      return res;
    }
  } else {// specular light
    
    // important sample GGX
    // pdf = D * cos(theta) * sin(theta)
    float a = roughness * roughness;
    float theta = acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
    float phi = 2.0 * PI * random.x;
    
    vec3 localH = dir_from_thetaphi(theta, phi);
    vec3 H = tbn_local * localH;  
    vec3 L = reflect(-V, H);

    // all required dot products
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);     
    
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
    
    contrib *= 2.0; // compensate for splitting diffuse and specular
    
    BSDFSample res;
    res.contribution = contrib;
    res.direction = normalize(L);
    res.pdf = 1.0;
    res.specular = true;
    return res;
  } 
}