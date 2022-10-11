#include "ray_tracing_common.glsl"

#ifndef GLSL_RAY_TRACING_GLSL
#define GLSL_RAY_TRACING_GLSL

#define rayPayload rayPayloadEXT
#define rayPayloadIn rayPayloadInEXT
#define hitAttribute hitAttributeEXT
#define callableData callableDataEXT
#define callableDataIn callableDataInEXT
#define shaderRecord shaderRecordEXT

#define gl_LaunchID gl_LaunchIDEXT
#define gl_LaunchSize gl_LaunchSizeEXT
#define gl_InstanceCustomIndex gl_InstanceCustomIndexEXT
#define gl_GeometryIndex gl_GeometryIndexEXT
#define gl_WorldRayOrigin gl_WorldRayOriginEXT
#define gl_WorldRayDirection gl_WorldRayDirectionEXT
#define gl_ObjectRayOrigin gl_ObjectRayOriginEXT
#define gl_ObjectRayDirection gl_ObjectRayDirectionEXT
#define gl_RayTmin gl_RayTminEXT
#define gl_RayTmax gl_RayTmaxEXT
#define gl_IncomingRayFlags gl_IncomingRayFlagsEXT
#define gl_HitT gl_HitTEXT
#define gl_HitKind gl_HitKindEXT
#define gl_ObjectToWorld gl_ObjectToWorldEXT
#define gl_WorldToObject gl_WorldToObjectEXT
#define gl_WorldToObject3x4 gl_WorldToObject3x4EXT
#define gl_ObjectToWorld3x4 gl_ObjectToWorld3x4EXT

#define gl_HitKindFrontFacingTriangle gl_HitKindFrontFacingTriangleEXT
#define gl_HitKindBackFacingTriangle gl_HitKindBackFacingTriangleEXT
#define traceRay traceRayEXT
#define reportIntersection reportIntersectionEXT
#define ignoreIntersection ignoreIntersectionEXT
#define terminateRay terminateRayEXT
#define executeCallable executeCallableEXT
#define nonuniform nonuniformEXT

// Ray falgs
#define gl_RayFlagsNone gl_RayFlagsNoneEXT
#define gl_RayFlagsOpaque gl_RayFlagsOpaqueEXT
#define gl_RayFlagsNoOpaque gl_RayFlagsNoOpaqueEXT
#define gl_RayFlagsTerminateOnFirstHit gl_RayFlagsTerminateOnFirstHitEXT
#define gl_RayFlagsSkipClosestHitShader gl_RayFlagsSkipClosestHitShaderEXT
#define gl_RayFlagsCullBackFacingTriangles gl_RayFlagsCullBackFacingTrianglesEXT
#define gl_RayFlagsCullFrontFacingTriangles gl_RayFlagsCullFrontFacingTrianglesEXT
#define gl_RayFlagsCullOpaque gl_RayFlagsCullOpaqueEXT
#define gl_RayFlagsCullNoOpaque gl_RayFlagsCullNoOpaqueEXT

#endif // GLSL_RAY_TRACING_GLSL