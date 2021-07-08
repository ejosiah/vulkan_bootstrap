#ifndef POINT_GENERATOR_GLSL
#define POINT_GENERATOR_GLSL

#define GRID_POINT_GENERATOR 0
#define BCC_LATTICE_POINT_GENERATOR 1
#define FCC_LATTICE_POINT_GENERATOR 2

vec4 gridPoint(vec3 lowerConer, vec3 upperConer, float spacing, uvec3 id){
    return vec4(
        id.x * spacing + lowerConer.x,
        id.y * spacing + lowerConer.y,
        id.z * spacing + lowerConer.z,
        1
    );
}

vec4 bccLatticePoint(vec3 lowerConer, vec3 upperConer, float spacing, uvec3 id){

    float halfSpacing = spacing * 0.5;
    float offset = (id.z % 2 == 1) ? halfSpacing : 0;
    return vec4(
        id.x * spacing + offset + lowerConer.x,
        id.y * spacing + offset + lowerConer.y,
        id.z * halfSpacing + lowerConer.z,
        1
    );
}

vec4 fccLatticePoint(vec3 lowerConer, vec3 upperConer, float spacing, uvec3 id){

    float halfSpacing = spacing * 0.5;
    float offset = ((id.z % 2 == 1 && id.y % 2 == 0) || (id.y % 2 == 1)) ? halfSpacing : 0;
    return vec4(
        id.x * spacing + offset + lowerConer.x,
        id.y * halfSpacing + lowerConer.y,
        id.z * halfSpacing + lowerConer.z,
        1
    );
}

vec4 generatePoint(vec3 lowerConer, vec3 upperConer, float spacing, int genType, uvec3 id){
    switch(genType){
        case GRID_POINT_GENERATOR:
            return gridPoint(lowerConer, upperConer, spacing, id);
        case BCC_LATTICE_POINT_GENERATOR:
            return bccLatticePoint(lowerConer, upperConer, spacing, id);
        case FCC_LATTICE_POINT_GENERATOR:
            return fccLatticePoint(lowerConer, upperConer, spacing, id);
        default:
            return vec4(0);
    }
}

bool shouldExit(vec3 lowerConer, vec3 upperConer, float spacing, int genType, uvec3 id){
    vec3 boxSize = abs(lowerConer - upperConer);
    float halfSpacing = spacing * 0.5;
    float offset = 0;

    switch(genType){
        case GRID_POINT_GENERATOR:
            return (id.z * spacing > boxSize.z) || (id.y * spacing > boxSize.y) || (id.x * spacing > boxSize.x);
        case BCC_LATTICE_POINT_GENERATOR:
            offset = (id.z % 2 == 1) ? halfSpacing : 0;
            return (id.z * halfSpacing > boxSize.z) || (id.y * spacing + offset > boxSize.y) || (id.x * spacing + offset > boxSize.x);
        case FCC_LATTICE_POINT_GENERATOR:
          offset = ((id.z % 2 == 1 && id.y % 2 == 0) || (id.y % 2 == 1)) ? halfSpacing : 0;
            return (id.z * halfSpacing > boxSize.z) || (id.y * halfSpacing > boxSize.y) || (id.x * spacing + offset > boxSize.x);
        default:
            return false;
    }
}

#endif // POINT_GENERATOR_GLSL