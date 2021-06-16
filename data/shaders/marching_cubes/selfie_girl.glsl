
// This SDF is really 6 braids at once (through domain
// repetition) with three strands each (brute forced)
vec4 sdHair( vec3 p, vec3 pa, vec3 pb, vec3 pc, float an, out vec2 occ_id)
{
    vec4 b = sdBezier(p, pa,pb,pc );
    vec2 q = rot(b.zw,an);

    vec2 id2 = round(q/0.1);
    id2 = clamp(id2,vec2(0),vec2(2,1));
    q -= 0.1*id2;

    float id = 11.0*id2.x + id2.y*13.0;

    q += smoothstep(0.5,0.8,b.y)*0.02*vec2(0.4,1.5)*cos( 23.0*b.y + id*vec2(13,17));

    occ_id.x = clamp(length(q)*8.0-0.2,0.0,1.0);
    vec4 res = vec4(99,q,b.y);
    for( int i=0; i<3; i++ )
    {
        vec2 tmp = q + 0.01*cos( id + 180.0*b.y + vec2(2*i,6-2*i));
        float lt = length(tmp)-0.02;
        if( lt<res.x )
        {
            occ_id.y = id+float(i);
            res.x = lt;
            res.yz = tmp;
        }
    }
    return res;
}

// The SDF for the hoodie and jacket. It's a very distorted
// ellipsoid, torus section, a segment and a sphere.
vec4 sdHoodie( in vec3 pos )
{
    vec3 opos = pos;

    pos.x   += 0.09*sin(3.5*pos.y-0.5)*sin(    pos.z) + 0.015;
    pos.xyz += 0.03*sin(2.0*pos.y)*sin(7.0*pos.zyx);

    // hoodie
    vec3 hos = pos-vec3(0.0,-0.33,0.15);
    hos.x -= 0.031*smoothstep(0.0,1.0,opos.y+0.33);
    hos.yz = rot(hos.yz,0.9);
    float d1 = sdEllipsoid(hos,vec3(0.96-pos.y*0.1,1.23,1.5));
    float d2 = 0.95*pos.z-0.312*pos.y-0.9;
    float d = max(opOnion(d1,0.01), d2 );

    // shoulders
    vec3 sos = vec3( abs(pos.x), pos.yz );
    vec2 se = sdSegment(sos, vec3(0.18,-1.6,-0.3), vec3(1.1,-1.9,0.0) );
    d = smin(d,se.x-mix(0.25,0.43,se.y),0.4);
    d = smin(d,sdSphere(sos-vec3(0.3,-2.2,0.4), 0.5 ),0.2);

    // neck
    opos.x -= 0.02*sin(9.0*opos.y);
    vec4 w = opElongate( opos-vec3(0.0,-1.2,0.3), vec3(0.0,0.3,0.0) );
    d = smin(d,
    w.w+sdCappedTorus(vec3(w.xy,-w.z),vec2(0.6,-0.8),0.6,0.02),
    0.1);

    // bumps
    d += 0.004*sin(pos.x*90.0)*sin(pos.y*90.0)*sin(pos.z*90.0);
    d -= 0.002*sin(pos.x*300.0);
    d -= 0.02*(1.0-smoothstep(0.0,0.04,abs(opOnion(pos.x,1.1))));

    // border
    d = min(d,length(vec2(d1,d2))-0.015);

    return vec4(d,pos);
}

// moves the head (and hair and hoodie). This could be done
// more efficiently (with a single matrix or quaternion),
// but this code was optimized for editing, not for runtime
vec3 moveHead( in vec3 pos, in vec3 an, in float amount)
{
    pos.y -= -1.0;
    pos.xz = rot(pos.xz,amount*an.x);
    pos.xy = rot(pos.xy,amount*an.y);
    pos.yz = rot(pos.yz,amount*an.z);
    pos.y += -1.0;
    return pos;
}

// the animation state

vec3 animData; // { blink, nose follow up, mouth }
vec3 animHead; // { head rotation angles }

vec4 map( in vec3 pos, in float time, out float outMat, out vec3 uvw )
{
    outMat = 1.0;

    vec3 oriPos = pos;

    // head deformation and transformation
    pos.y /= 1.04;
    vec3 opos;
    opos = moveHead( pos, animHead, smoothstep(-1.2, 0.2,pos.y) );
    pos  = moveHead( pos, animHead, smoothstep(-1.4,-1.0,pos.y) );
    pos.x *= 1.04;
    pos.y /= 1.02;
    uvw = pos;

    // symmetric coord systems (sharp, and smooth)
    vec3 qos = vec3(abs(pos.x),pos.yz);
    vec3 sos = vec3(sqrt(qos.x*qos.x+0.0005),pos.yz);

    // head
    float d = sdEllipsoid( pos-vec3(0.0,0.05,0.07), vec3(0.8,0.75,0.85) );

    // jaw
    vec3 mos = pos-vec3(0.0,-0.38,0.35); mos.yz = rot(mos.yz,0.4);
    mos.yz = rot(mos.yz,0.1*animData.z);
    float d2 = sdEllipsoid(mos-vec3(0,-0.17,0.16),
    vec3(0.66+sclamp(mos.y*0.9-0.1*mos.z,-0.3,0.4),
    0.43+sclamp(mos.y*0.5,-0.5,0.2),
    0.50+sclamp(mos.y*0.3,-0.45,0.5)));

    // mouth hole
    d2 = smax(d2,-sdEllipsoid(mos-vec3(0,0.06,0.6+0.05*animData.z), vec3(0.16,0.035+0.05*animData.z,0.1)),0.01);

    // lower lip
    vec4 b = sdBezier(vec3(abs(mos.x),mos.yz),
    vec3(0,0.01,0.61),
    vec3(0.094+0.01*animData.z,0.015,0.61),
    vec3(0.18-0.02*animData.z,0.06+animData.z*0.05,0.57-0.006*animData.z));
    float isLip = smoothstep(0.045,0.04,b.x+b.y*0.03);
    d2 = smin(d2,b.x - 0.027*(1.0-b.y*b.y)*smoothstep(1.0,0.4,b.y),0.02);
    d = smin(d,d2,0.19);

    // chicks
    d = smin(d,sdSphere(qos-vec3(0.2,-0.33,0.62),0.28 ),0.04);

    // who needs ears

    // eye sockets
    vec3 eos = sos-vec3(0.3,-0.04,0.7);
    eos.xz = rot(eos.xz,-0.2);
    eos.xy = rot(eos.xy,0.3);
    eos.yz = rot(eos.yz,-0.2);
    d2 = sdEllipsoid( eos-vec3(-0.05,-0.05,0.2), vec3(0.20,0.14-0.06*animData.x,0.1) );
    d = smax( d, -d2, 0.15 );

    eos = sos-vec3(0.32,-0.08,0.8);
    eos.xz = rot(eos.xz,-0.4);
    d2 = sdEllipsoid( eos, vec3(0.154,0.11,0.1) );
    d = smax( d, -d2, 0.05 );

    vec3 oos = qos - vec3(0.25,-0.06,0.42);

    // eyelid
    d2 = sdSphere( oos, 0.4 );
    oos.xz = rot(oos.xz, -0.2);
    oos.xy = rot(oos.xy, 0.2);
    vec3 tos = oos;
    oos.yz = rot(oos.yz,-0.6+0.58*animData.x);

    //eyebags
    tos = tos-vec3(-0.02,0.06,0.2+0.02*animData.x);
    tos.yz = rot(tos.yz,0.8);
    tos.xy = rot(tos.xy,-0.2);
    d = smin( d, sdTorus(tos,0.29,0.01), 0.03 );

    // eyelids
    eos = qos - vec3(0.33,-0.07,0.53);
    eos.xy = rot(eos.xy, 0.2);
    eos.yz = rot(eos.yz,0.35-0.25*animData.x);
    d2 = smax(d2-0.005, -max(oos.y+0.098,-eos.y-0.025), 0.02 );
    d = smin( d, d2, 0.012 );

    // eyelashes
    oos.x -= 0.01;
    float xx = clamp( oos.x+0.17,0.0,1.0);
    float ra = 0.35 + 0.1*sqrt(xx/0.2)*(1.0-smoothstep(0.3,0.4,xx))*(0.6+0.4*sin(xx*256.0));
    float rc = 0.18/(1.0-0.7*smoothstep(0.15,0.5,animData.x));
    oos.y -= -0.18 - (rc-0.18)/1.8;
    d2 = (1.0/1.8)*sdArc( oos.xy*vec2(1.0,1.8), vec2(0.9,sqrt(1.0-0.9*0.9)), rc )-0.001;
    float deyelashes = max(d2,length(oos.xz)-ra)-0.003;

    // nose
    eos = pos-vec3(0.0,-0.079+animData.y*0.005,0.86);
    eos.yz = rot(eos.yz,-0.23);
    float h = smoothstep(0.0,0.26,-eos.y);
    d2 = sdCone( eos-vec3(0.0,-0.02,0.0), vec2(0.03,-0.25) )-0.04*h-0.01;
    eos.x = sqrt(eos.x*eos.x + 0.001);
    d2 = smin( d2, sdSphere(eos-vec3(0.0, -0.25,0.037),0.06 ), 0.07 );
    d2 = smin( d2, sdSphere(eos-vec3(0.1, -0.27,0.03 ),0.04 ), 0.07 );
    d2 = smin( d2, sdSphere(eos-vec3(0.0, -0.32,0.05 ),0.025), 0.04 );
    d2 = smax( d2,-sdSphere(eos-vec3(0.07,-0.31,0.038),0.02 ), 0.035 );
    d = smin(d,d2,0.05-0.03*h);

    // mouth
    eos = pos-vec3(0.0,-0.38+animData.y*0.003+0.01*animData.z,0.71);
    tos = eos-vec3(0.0,-0.13,0.06);
    tos.yz = rot(tos.yz,0.2);
    float dTeeth = sdTorus(tos,0.15,0.015);
    eos.yz = rot(eos.yz,-0.5);
    eos.x /= 1.04;

    // nose-to-upperlip connection
    d2 = sdCone( eos-vec3(0,0,0.03), vec2(0.14,-0.2) )-0.03;
    d2 = max(d2,-(eos.z-0.03));
    d = smin(d,d2,0.05);

    // upper lip
    eos.x = abs(eos.x);
    b = sdBezier(eos, vec3(0.00,-0.22,0.17),
    vec3(0.08,-0.22,0.17),
    vec3(0.17-0.02*animData.z,-0.24-0.01*animData.z,0.08));
    d2 = length(b.zw/vec2(0.5,1.0)) - 0.03*clamp(1.0-b.y*b.y,0.0,1.0);
    d = smin(d,d2,0.02);
    isLip = max(isLip,(smoothstep(0.03,0.005,abs(b.z+0.015+abs(eos.x)*0.04))
    -smoothstep(0.45,0.47,eos.x-eos.y*1.15)));

    // valley under nose
    vec2 se = sdSegment(pos, vec3(0.0,-0.45,1.01),  vec3(0.0,-0.47,1.09) );
    d2 = se.x-0.03-0.06*se.y;
    d = smax(d,-d2,0.04);
    isLip *= smoothstep(0.01,0.03,d2);

    // neck
    se = sdSegment(pos, vec3(0.0,-0.65,0.0), vec3(0.0,-1.7,-0.1) );
    d2 = se.x - 0.38;

    // shoulders
    se = sdSegment(sos, vec3(0.0,-1.55,0.0), vec3(0.6,-1.65,0.0) );
    d2 = smin(d2,se.x-0.21,0.1);
    d = smin(d,d2,0.4);

    // register eyelases now
    vec4 res = vec4( d, isLip, 0, 0 );
    if( deyelashes<res.x )
    {
        res.x = deyelashes*0.8;
        res.yzw = vec3(0.0,1.0,0.0);
    }
    // register teeth now
    if( dTeeth<res.x )
    {
        res.x = dTeeth;
        outMat = 5.0;
    }

    // eyes
    pos.x /=1.05;
    eos = qos-vec3(0.25,-0.06,0.42);
    d2 = sdSphere(eos,0.4);
    if( d2<res.x )
    {
        res.x = d2;
        outMat = 2.0;
        uvw = pos;
    }

    // hair
    {
        vec2 occ_id, tmp;
        qos = pos; qos.x=abs(pos.x);

        vec4 pres = sdHair(pos,vec3(-0.3, 0.55,0.8),
        vec3( 0.95, 0.7,0.85),
        vec3( 0.4,-1.45,0.95),
        -0.9,occ_id);

        vec4 pres2 = sdHair(pos,vec3(-0.4, 0.6,0.55),
        vec3(-1.0, 0.4,0.2),
        vec3(-0.6,-1.4,0.7),
        0.6,tmp);
        if( pres2.x<pres.x ) { pres=pres2; occ_id=tmp;  occ_id.y+=40.0;}

        pres2 = sdHair(qos,vec3( 0.4, 0.7,0.4),
        vec3( 1.0, 0.5,0.45),
        vec3( 0.4,-1.45,0.55),
        -0.2,tmp);
        if( pres2.x<pres.x ) { pres=pres2; occ_id=tmp;  occ_id.y+=80.0;}


        pres.x *= 0.8;
        if( pres.x<res.x )
        {
            res = vec4( pres.x, occ_id.y, 0.0, occ_id.x );
            uvw = pres.yzw;
            outMat = 4.0;
        }
    }

    // hoodie
    vec4 tmp = sdHoodie( opos );
    if( tmp.x<res.x )
    {
        res.x = tmp.x;
        outMat = 3.0;
        uvw  = tmp.yzw;
    }

    return res;
}