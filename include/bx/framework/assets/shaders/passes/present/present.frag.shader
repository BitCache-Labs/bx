#include "[engine]/shaders/Language.shader"

layout (location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(BINDING(0, 0)) uniform texture2D colorImage;
layout (BINDING(0, 1)) uniform sampler linearClampSampler;

void main()
{
    vec3 color = texture(sampler2D(colorImage, linearClampSampler), fragTexCoord).rgb;
    outColor = vec4(color, 1.0);
}