#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

float calc_point_light_attenuation(vec3 light_position, float radius, vec3 frag_position, out vec3 L)
{
    const float rmin = 0.0001;

    vec3 frag_pos_to_light = light_position - frag_position;
    float r = length(frag_pos_to_light);
    L = frag_pos_to_light / r;

    float attenuation = pow(radius / max(r, rmin), 2);
    attenuation *= pow(max(1 - pow(r / radius, 4), 0.0), 2);
    return attenuation;
}

float calc_spot_light_attenuation(vec3 light_position, float radius, vec3 direction, float outer_angle, float inner_angle, vec3 frag_position, out vec3 L)
{
    const float rmin = 0.0001;
    vec3 frag_pos_to_light = light_position - frag_position;
    float r = length(frag_pos_to_light);
    L = frag_pos_to_light / r;

    float attenuation = pow(radius / max(r, rmin), 2);
    attenuation *= pow(max(1 - pow(r / radius, 4), 0.0), 2);

    float cos_theta_u = cos(outer_angle);
    float cos_theta_p = cos(inner_angle);
    float cos_theta_s = dot(direction, -L);
    float t = pow(clamp((cos_theta_s - cos_theta_u) / (cos_theta_p - cos_theta_u), 0.0, 1.0), 2.0);
    attenuation *= t;

    return attenuation;
}

#endif