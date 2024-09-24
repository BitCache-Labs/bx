#include "[engine]/shaders/Language.shader"

layout (location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(BINDING(0, 0)) uniform sampler2D colorImage;

vec3 aces(vec3 x)
{
    return x;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 gammaCorrect(vec3 x, float gamma)
{
    return pow(x, vec3(1.0 / gamma));
}

void main()
{
    vec3 hdrColor = texture(colorImage, fragTexCoord).rgb;

    // acesg -> acescct (matrix mult)

    // 2 luts, 1d & 3d textures
    // acescct , apply output transform (monitor dependant)
    
    vec3 sdrColor = aces(hdrColor);
    sdrColor = gammaCorrect(sdrColor, 2.2);

    outColor = vec4(sdrColor, 1.0);
}