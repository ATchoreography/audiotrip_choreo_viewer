#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

struct MaterialProperty {
    vec3 color;
    int useSampler;
    sampler2D sampler;
};

uniform vec4 ambient;
uniform vec3 viewPos;

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture2D(texture0, fragTexCoord);
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);

    // I have no idea what I'm doing but I like it better
    texelColor *= 5;

    // NOTE: Implement here your fragment shader code
    vec4 finalColor = texelColor*(colDiffuse*vec4(lightDot, 1.0));
    finalColor += texelColor*(ambient/10.0);

    // Gamma correction
    gl_FragColor = pow(finalColor, vec4(1.0/2.2));
}
