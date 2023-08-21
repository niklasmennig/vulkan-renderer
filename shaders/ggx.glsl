#include "common.glsl"

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

vec3 ggx(vec3 ray_in, vec3 ray_out, mat3 tbn, vec3 base_color, float metallic, float fresnel_reflect, float roughness, float transmission, float ior) {
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

    return diff + spec;
}

vec3 sample_ggx(in vec3 V, in mat3 tbn, 
              in vec3 baseColor, float opacity, in float metallicness, 
              in float fresnelReflect, in float roughness, in float transmission, in float ior, in vec4 random, out vec3 nextFactor) 
{
    vec3 N = tbn[2];

    // handle opacity
    if (random.w > opacity) {
      nextFactor = vec3(1.0);
      return -V;
    }

    if(random.z < 0.5) { // non-specular light
    if((2.0 * random.z) < transmission) { // transmitted light
      vec3 forwardNormal = N;
      float frontFacing = dot(V, N);
      float eta =1.0 / ior;
      if(frontFacing < 0.0) {
         forwardNormal = -N;
         tbn[2] = forwardNormal;
         eta = ior;
      } 
      
      // important sample GGX
      // pdf = D * cos(theta) * sin(theta)
      float a = roughness * roughness;
      float theta = acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
      float phi = 2.0 * PI * random.x;
      
      vec3 localH = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      vec3 H = tbn * localH;  
      
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
      nextFactor = baseColor * (vec3(1.0) - F) * G * VoH / max((NoH * NoV), 0.001);
    
      nextFactor *= 2.0; // compensate for splitting diffuse and specular
      return L;
      
    } else { // diffuse light
      
      // important sampling diffuse
      // pdf = cos(theta) * sin(theta) / PI
      float theta = asin(sqrt(random.y));
      float phi = 2.0 * PI * random.x;
      // sampled indirect diffuse direction in normal space
      vec3 localDiffuseDir = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      vec3 L = tbn * localDiffuseDir;  
      
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
    
      nextFactor = notSpec * baseColor;
      nextFactor *= 2.0; // compensate for splitting diffuse and specular
      return L;
    }
  } else {// specular light
    
    // important sample GGX
    // pdf = D * cos(theta) * sin(theta)
    float a = roughness * roughness;
    float theta = acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
    float phi = 2.0 * PI * random.x;
    
    vec3 localH = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    vec3 H = basis(N) * localH;  
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
    nextFactor =  F * G * VoH / max((NoH * NoV), 0.001);
    
    nextFactor *= 2.0; // compensate for splitting diffuse and specular
    return L;
  } 
  
}