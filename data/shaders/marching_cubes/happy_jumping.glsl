#ifndef HAPPY_JUMPING_GLSL
#define HAPPY_JUMPING_GLSL

#include "sdf_common.glsl"


float href;
float hsha;

vec4 map( in vec3 pos, float atime )
{
    hsha = 1.0;

    float t1 = fract(atime);
    float t4 = abs(fract(atime*0.5)-0.5)/0.5;

    float p = 4.0*t1*(1.0-t1);
    float pp = 4.0*(1.0-2.0*t1); // derivative of p

    vec3 cen = vec3( 0.5*(-1.0 + 2.0*t4),
    pow(p,2.0-p) + 0.1,
    floor(atime) + pow(t1,0.7) -1.0 );

    // body
    vec2 uu = normalize(vec2( 1.0, -pp ));
    vec2 vv = vec2(-uu.y, uu.x);

    float sy = 0.5 + 0.5*p;
    float compress = 1.0-smoothstep(0.0,0.4,p);
    sy = sy*(1.0-compress) + compress;
    float sz = 1.0/sy;

    vec3 q = pos - cen;
    float rot = -0.25*(-1.0 + 2.0*t4);
    float rc = cos(rot);
    float rs = sin(rot);
    q.xy = mat2x2(rc,rs,-rs,rc)*q.xy;
    vec3 r = q;
    href = q.y;
    q.yz = vec2( dot(uu,q.yz), dot(vv,q.yz) );

    float deli = sdEllipsoid( q, vec3(0.25, 0.25*sy, 0.25*sz) );
    vec4 res = vec4( deli, 2.0, 0.0, 1.0 );


    // ground
    float fh = -0.1 - 0.05*(sin(pos.x*2.0)+sin(pos.z*2.0));
    float t5f = fract(atime+0.05);
    float t5i = floor(atime+0.05);
    float bt4 = abs(fract(t5i*0.5)-0.5)/0.5;
    vec2  bcen = vec2( 0.5*(-1.0+2.0*bt4),t5i+pow(t5f,0.7)-1.0 );

    float k = length(pos.xz-bcen);
    float tt = t5f*15.0-6.2831 - k*3.0;
    fh -= 0.1*exp(-k*k)*sin(tt)*exp(-max(tt,0.0)/2.0)*smoothstep(0.0,0.01,t5f);
    float d = pos.y - fh;

    // bubbles
    {
        vec3 vp = vec3( mod(abs(pos.x),3.0)-1.5,pos.y,mod(pos.z+1.5,3.0)-1.5);
        vec2 id = vec2( floor(pos.x/3.0), floor((pos.z+1.5)/3.0) );
        float fid = id.x*11.1 + id.y*31.7;
        float fy = fract(fid*1.312+atime*0.1);
        float y = -1.0+4.0*fy;
        vec3  rad = vec3(0.7,1.0+0.5*sin(fid),0.7);
        rad -= 0.1*(sin(pos.x*3.0)+sin(pos.y*4.0)+sin(pos.z*5.0));
        float siz = 4.0*fy*(1.0-fy);
        float d2 = sdEllipsoid( vp-vec3(0.5,y,0.0), siz*rad );

        d2 -= 0.03*smoothstep(-1.0,1.0,sin(18.0*pos.x)+sin(18.0*pos.y)+sin(18.0*pos.z));
        d2 *= 0.6;
        d2 = min(d2,2.0);
        d = smin( d, d2, 0.32 );
        if( d<res.x ) { res = vec4(d,1.0,0.0,1.0); hsha=sqrt(siz); }
    }

    // rest of body
    if( deli-1.0 < res.x ) // bounding volume
    {
        float t2 = fract(atime+0.8);
        float p2 = 0.5-0.5*cos(6.2831*t2);
        r.z += 0.05-0.2*p2;
        r.y += 0.2*sy-0.2;
        vec3 sq = vec3( abs(r.x), r.yz );

        // head
        vec3 h = r;
        float hr = sin(0.791*atime);
        hr = 0.7*sign(hr)*smoothstep(0.5,0.7,abs(hr));
        h.xz = mat2x2(cos(hr),sin(hr),-sin(hr),cos(hr))*h.xz;
        vec3 hq = vec3( abs(h.x), h.yz );
        float d  = sdEllipsoid( h-vec3(0.0,0.20,0.02), vec3(0.08,0.2,0.15) );
        float d2 = sdEllipsoid( h-vec3(0.0,0.21,-0.1), vec3(0.20,0.2,0.20) );
        d = smin( d, d2, 0.1 );
        res.x = smin( res.x, d, 0.1 );

        // belly wrinkles
        {
            float yy = r.y-0.02-2.5*r.x*r.x;
            res.x += 0.001*sin(yy*120.0)*(1.0-smoothstep(0.0,0.1,abs(yy)));
        }

        // arms
        {
            vec2 arms = sdStick( sq, vec3(0.18-0.06*hr*sign(r.x),0.2,-0.05), vec3(0.3+0.1*p2,-0.2+0.3*p2,-0.15), 0.03, 0.06 );
            res.xz = smin( res.xz, arms, 0.01+0.04*(1.0-arms.y)*(1.0-arms.y)*(1.0-arms.y) );
        }

        // ears
        {
            float t3 = fract(atime+0.9);
            float p3 = 4.0*t3*(1.0-t3);
            vec2 ear = sdStick( hq, vec3(0.15,0.32,-0.05), vec3(0.2+0.05*p3,0.2+0.2*p3,-0.07), 0.01, 0.04 );
            res.xz = smin( res.xz, ear, 0.01 );
        }

        // mouth
        {
            d = sdEllipsoid( h-vec3(0.0,0.15+4.0*hq.x*hq.x,0.15), vec3(0.1,0.04,0.2) );
            res.w = 0.3+0.7*clamp( d*150.0,0.0,1.0);
            res.x = smax( res.x, -d, 0.03 );
        }

        // legs
        {
            float t6 = cos(6.2831*(atime*0.5+0.25));
            float ccc = cos(1.57*t6*sign(r.x));
            float sss = sin(1.57*t6*sign(r.x));
            vec3 base = vec3(0.12,-0.07,-0.1); base.y -= 0.1/sy;
            vec2 legs = sdStick( sq, base, base + vec3(0.2,-ccc,sss)*0.2, 0.04, 0.07 );
            res.xz = smin( res.xz, legs, 0.07 );
        }

        // eye
        {
            float blink = pow(0.5+0.5*sin(2.1*iTime),20.0);
            float eyeball = sdSphere(hq-vec3(0.08,0.27,0.06),0.065+0.02*blink);
            res.x = smin( res.x, eyeball, 0.03 );

            vec3 cq = hq-vec3(0.1,0.34,0.08);
            cq.xy = mat2x2(0.8,0.6,-0.6,0.8)*cq.xy;
            d = sdEllipsoid( cq, vec3(0.06,0.03,0.03) );
            res.x = smin( res.x, d, 0.03 );

            float eo = 1.0-0.5*smoothstep(0.01,0.04,length((hq.xy-vec2(0.095,0.285))*vec2(1.0,1.1)));
            res = opU( res, vec4(sdSphere(hq-vec3(0.08,0.28,0.08),0.060),3.0,0.0,eo));
            res = opU( res, vec4(sdSphere(hq-vec3(0.075,0.28,0.102),0.0395),4.0,0.0,1.0));
        }
    }


    // candy
    if( pos.y-1.0 < res.x ) // bounding volume
    {
        float fs = 5.0;
        vec3 qos = fs*vec3(pos.x, pos.y-fh, pos.z );
        vec2 id = vec2( floor(qos.x+0.5), floor(qos.z+0.5) );
        vec3 vp = vec3( fract(qos.x+0.5)-0.5,qos.y,fract(qos.z+0.5)-0.5);
        vp.xz += 0.1*cos( id.x*130.143 + id.y*120.372 + vec2(0.0,2.0) );
        float den = sin(id.x*0.1+sin(id.y*0.091))+sin(id.y*0.1);
        float fid = id.x*0.143 + id.y*0.372;
        float ra = smoothstep(0.0,0.1,den*0.1+fract(fid)-0.95);
        d = sdSphere( vp, 0.35*ra )/fs;
        if( d<res.x ) res = vec4(d,5.0,qos.y,1.0);
    }

    return res;
}

vec3 calcNormal( in vec3 pos, float time )
{

    #if 0
    vec2 e = vec2(1.0,-1.0)*0.5773*0.001;
    return normalize( e.xyy*map( pos + e.xyy, time ).x +
    e.yyx*map( pos + e.yyx, time ).x +
    e.yxy*map( pos + e.yxy, time ).x +
    e.xxx*map( pos + e.xxx, time ).x );
    #else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+0.001*e,time).x;
    }
    return normalize(n);
    #endif
}

#endif // HAPPY_JUMPING_GLSL