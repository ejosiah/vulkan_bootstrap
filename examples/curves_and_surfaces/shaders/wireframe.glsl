vec3 edgeDistance(vec3 p0, vec3 p1, vec3 p2){
    float a = distance(p1, p2);
    float b = distance(p0, p1);
    float c = distance(p0, p2);

    float alpha = acos((b * b + c * c - a * a)/(2 * b * c));
    float beta =  acos((a * a + c * c - b * b)/(2 * a * c));

    float ha = abs(c * sin(beta));
    float hb = abs(c * sin(alpha));
    float hc = abs(b * sin(alpha));

    return vec3(ha, hb, hc);
}