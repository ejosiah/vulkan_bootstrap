#ifndef SDF_COMMON_GLSL
#define SDF_COMMON_GLSL

// http://iquilezles.org/www/articles/smin/smin.htm
float smin( float a, float b, float k )
{
    float h = max(k-abs(a-b),0.0);
    return min(a, b) - h*h*0.25/k;
}

// http://iquilezles.org/www/articles/smin/smin.htm
vec2 smin( vec2 a, vec2 b, float k )
{
    float h = clamp( 0.5+0.5*(b.x-a.x)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

// http://iquilezles.org/www/articles/smin/smin.htm
float smax( float a, float b, float k )
{
    float h = max(k-abs(a-b),0.0);
    return max(a, b) + h*h*0.25/k;
}

// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float sdSphere( vec3 p, float s )
{
    return length(p)-s;
}

// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float sdEllipsoid( in vec3 p, in vec3 r ) // approximated
{
    float k0 = length(p/r);
    float k1 = length(p/(r*r));
    return k0*(k0-1.0)/k1;
}

vec2 sdStick(vec3 p, vec3 a, vec3 b, float r1, float r2) // approximated
{
    vec3 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return vec2( length( pa - ba*h ) - mix(r1,r2,h*h*(3.0-2.0*h)), h );
}

// http://iquilezles.org/www/articles/smin/smin.htm
vec4 opU( vec4 d1, vec4 d2 )
{
    return (d1.x<d2.x) ? d1 : d2;
}

float smin( float a, float b )
{
    return smin(a, b, 0.03);
}

    #if 1
// http://research.microsoft.com/en-us/um/people/hoppe/ravg.pdf
// { dist, t, y (above the plane of the curve, x (away from curve in the plane of the curve))
vec4 sdBezier( vec3 p, vec3 va, vec3 vb, vec3 vc )
{
    vec3 w = normalize( cross( vc-vb, va-vb ) );
    vec3 u = normalize( vc-vb );
    vec3 v =          ( cross( w, u ) );
    //----
    vec2 m = vec2( dot(va-vb,u), dot(va-vb,v) );
    vec2 n = vec2( dot(vc-vb,u), dot(vc-vb,v) );
    vec3 q = vec3( dot( p-vb,u), dot( p-vb,v), dot(p-vb,w) );
    //----
    float mn = det(m,n);
    float mq = det(m,q.xy);
    float nq = det(n,q.xy);
    //----
    vec2  g = (nq+mq+mn)*n + (nq+mq-mn)*m;
    float f = (nq-mq+mn)*(nq-mq+mn) + 4.0*mq*nq;
    vec2  z = 0.5*f*vec2(-g.y,g.x)/dot(g,g);
    //float t = clamp(0.5+0.5*(det(z,m+n)+mq+nq)/mn, 0.0 ,1.0 );
    float t = clamp(0.5+0.5*(det(z-q.xy,m+n))/mn, 0.0 ,1.0 );
    vec2 cp = m*(1.0-t)*(1.0-t) + n*t*t - q.xy;
    //----
    float d2 = dot(cp,cp);
    return vec4(sqrt(d2+q.z*q.z), t, q.z, -sign(f)*sqrt(d2) );
}
#else
float det( vec3 a, vec3 b, in vec3 v ) { return dot(v,cross(a,b)); }

#endif // SDF_COMMON_GLSL