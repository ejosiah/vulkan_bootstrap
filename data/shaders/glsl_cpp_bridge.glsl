#ifndef GLSL_CPP_BRIDGE_GLSL
#define GLSL_CPP_BRIDGE_GLSL

#ifdef __cplusplus
    #define IN(Type)    const Type&
    #define OUT(Type)   Type&
    #define INOUT(Type) Type&
#else
    #define IN(Type)    in Type
    #define OUT(Type)   out Type
    #define INOUT(Type) inout Type
#endif

#endif // GLSL_CPP_BRIDGE_GLSL