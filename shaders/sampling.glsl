struct BSDFSample {
    vec3 direction;
    float pdf;
};

BSDFSample sample_cosine_hemisphere(float u1, float u2)
{
    float theta = 0.5 * acos(-2.0 * u1 + 1.0);
    float phi = u2 * 2.0 * PI;

    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    return BSDFSample(
        vec3(x, y, z),
        1.0 / PI
    );
}

// taken from https://www.cim.mcgill.ca/~derek/ecse689_a3.html
BSDFSample sample_power_hemisphere(float u1, float u2, float n)
{
    float alpha = sqrt(1.0 - pow(u1, 2.0 / (n+1.0)));

    float x = alpha * cos(2.0 * PI * u2);
    float y = alpha * sin(2.0 * PI * u2);
    float z = pow(u1, 1.0 / (n+1.0));

    return BSDFSample(
        vec3(x, y, z),
        1.0 / PI
    );
}