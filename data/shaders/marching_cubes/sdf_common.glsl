#ifndef SDF_COMMON_GLSL
#define SDF_COMMON_GLSL

#define ZERO 0

float map(vec3 pos);

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 calcNormal( in vec3 pos )
{
    #if 0
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize( e.xyy*map( pos + e.xyy ).x +
    e.yyx*map( pos + e.yyx ).x +
    e.yxy*map( pos + e.yxy ).x +
    e.xxx*map( pos + e.xxx ).x );
    #else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+0.0005*e).x;
        //if( n.x+n.y+n.z>100.0 ) break;
    }
    return normalize(n);
    #endif
}

    #endif // SDF_COMMON_GLSL