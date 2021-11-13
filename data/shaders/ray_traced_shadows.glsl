#ifndef RAY_TRACED_SHADOWS
#define RAY_TRACED_SHADOWS

#include "ray_query_lang.glsl"
#include "hash.glsl"
#include "sampling.glsl"

bool InShadow(vec3 origin, vec3 direction, float dist, uint cullMask){

    rayQuery rQuery;
    rayQueryInitialize(rQuery, tlas, gl_RayFlagsTerminateOnFirstHit, cullMask, origin, 0.01, direction, dist);

    while(rayQueryProceed(rQuery)){
        if(rayQueryGetIntersectionType(rQuery, false) == gl_RayQueryCandidateIntersectionTriangle){
            // TODO Determine if an opaque triangle hit occurred
            bool opaqueHit = true;
            if(opaqueHit){
                rayQueryConfirmIntersection(rQuery);
            }
        }else if(rayQueryGetIntersectionType(rQuery, false) == gl_RayQueryCandidateIntersectionAABB){
            // TODO Determine if an opaque triangle hit occurred
            bool opaqueHit = true;
            if(opaqueHit){
                rayQueryConfirmIntersection(rQuery);
            }
        }
    }

    return rayQueryGetIntersectionType(rQuery, true) != gl_RayQueryCommittedIntersectionNone;
}

float shadow(vec3 objPos, vec3 lightPos, uint cullMask, int nSamples){
    vec3 lightDir = lightPos - objPos;
    float dist = length(lightDir);
    lightDir /= dist;
    vec3 N = lightDir;
    vec3 up = N.x > 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 right = cross(up, N);
    up = cross(N, right);

    float sum = 0;
    for(int i = 0; i < nSamples; i++){
        sum += InShadow(objPos, lightDir, dist, cullMask) ? 1 : 0;
        vec2 uv = hash23(lightDir);
//        vec2 uv = hammersleySquare(i, nSamples);
        lightDir = N + 0.3 * uniformSampleSphere(uv);
    }
    return sum/float(nSamples);
}


#endif // RAY_TRACED_SHADOWS