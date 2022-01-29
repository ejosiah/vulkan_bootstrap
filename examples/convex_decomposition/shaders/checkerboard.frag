#version 460

#define SQRT_3_INV 0.57735026918962576450914878050196

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 eyes;
} frag_in;

layout(location = 0) out vec4 fragColor;

vec3 GetColorFromPositionAndNormal( in vec3 worldPosition, in vec3 normal ) {
    const float pi = 3.141519;

    vec3 scaledPos = 2 * worldPosition.xyz * pi * 2.0;
    vec3 scaledPos2 = 2 * worldPosition.xyz * pi * 2.0 / 10.0 + vec3( pi / 4.0 );
    float s = cos( scaledPos2.x ) * cos( scaledPos2.y ) * cos( scaledPos2.z );  // [-1,1] range
    float t = cos( scaledPos.x ) * cos( scaledPos.y ) * cos( scaledPos.z );     // [-1,1] range


    t = ceil( t * 0.9 );
    s = ( ceil( s * 0.9 ) + 3.0 ) * 0.25;
    vec3 colorB = vec3( 0.85, 0.85, 0.85 );
    vec3 colorA = vec3( 1, 1, 1 );
    vec3 finalColor = mix( colorA, colorB, t ) * s;

    return vec3(0.8) * finalColor;
}

void main(){
    vec3 N = normalize(frag_in.normal);
    vec3 L = vec3(SQRT_3_INV);
    vec3 eyeDir = frag_in.eyes - frag_in.position;
    vec3 E = normalize(eyeDir);
    vec3 H = normalize(E + L);

    float shine = 200;
    vec3 color = GetColorFromPositionAndNormal(frag_in.position, frag_in.normal);

    float diffuse = max(0, dot(N, L));
    float specular = max(0, pow(dot(N, H), shine));
    vec3 radiance = color * diffuse;

    L.xz *= -1;
    diffuse = max(0, dot(N, L));
    specular = max(0, pow(dot(N, H), shine));
    radiance += color * (diffuse);

//    L = E;
//    H = normalize(E + L);
//    diffuse = max(0, dot(N, L));
//    specular = max(0, pow(dot(N, H), shine));
//    radiance += color * (diffuse);

    vec3 fog = vec3(0.8);
    float density = 0.05;
    float distFromEyes = length(eyeDir);
    float t = exp(-density * distFromEyes);
    t = clamp(t, 0, 1);
    vec3 finalColor = mix(fog, radiance, t);

    fragColor = vec4(finalColor, 0.3);
}