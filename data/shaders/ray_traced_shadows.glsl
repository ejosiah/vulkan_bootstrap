#ifndef RAY_TRACED_SHADOWS
#define RAY_TRACED_SHADOWS

#include "ray_query_lang.glsl"
#include "hash.glsl"
#include "sampling.glsl"

bool InShadow(vec3 origin, vec3 direction, float dist, uint cullMask){

    rayQuery rQuery;
    rayQueryInitialize(rQuery, tlas, gl_RayFlagsTerminateOnFirstHit, 0xFF, origin, 0.01, direction, dist);

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
    return InShadow(objPos, lightDir, dist, cullMask) ? 1 : 0;
}


#endif // RAY_TRACED_SHADOWS