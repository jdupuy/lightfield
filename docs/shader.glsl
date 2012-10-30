/*
 * Proland: a procedural landscape rendering library.
 * Copyright (c) 2008-2011 INRIA
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Proland is distributed under a dual-license scheme.
 * You can obtain a specific license from Inria: proland-licensing@inria.fr.
 */

/*
 * Authors: Eric Bruneton, Antoine Begault, Guillaume Piolat.
 */

uniform vec3 cameraRefPos; // xy: ref pos in horizontal plane, z: (1+(zFar+zNear)/(zFar-zNear))/2
uniform vec4 clip[4];
uniform mat4 localToTangentFrame;
uniform mat4 tangentFrameToScreen;
uniform mat4 tangentFrameToWorld;
uniform mat3 tangentSpaceToWorld;
uniform vec3 tangentSunDir;
uniform vec3 focalPos; // cameraPos is (0,0,focalPos.z) in tangent frame
uniform float plantRadius;
uniform float pass;



#ifdef _VERTEX_
bool isVisible(vec4 c, float r) {
    return dot(c, clip[0]) > - r && dot(c, clip[1]) > - r && dot(c, clip[2]) > - r && dot(c, clip[3]) > - r;
}
layout(location=0) in vec3 lPos; // tree position
layout(location=1) in vec3 lParams; //  tree params ?
out vec3 gPos;
out vec3 gParams;
out float generate;
void main()
{
    gPos = (localToTangentFrame * vec4(lPos - vec3(cameraRefPos.xy, 0.0), 1.0)).xyz;
    gParams = lParams;
    if (getTreeDensity() == 0.0) gParams.x = 1.0;
    float maxDist = getMaxTreeDistance();
    float r = gParams.x * plantRadius;
    generate = length(vec3(gPos.xy,gPos.z-focalPos.z)) < maxDist && isVisible(vec4(gPos + vec3(0.0, 0.0, r), 1.0), r) ? 1.0 : 0.0;
}
#endif



#ifdef _GEOMETRY_
layout(points) in;
layout(triangle_strip,max_vertices=4) out;
in vec3 gPos[];
in vec3 gParams[];
in float generate[];
flat out vec3 cPos; // camera position in local frame of cone (where cone center = 0,0,0, cone height = cone radius = 1)
flat out vec4 lDIR;
out vec4 cDir; // camera direction in local frame of cone (where cone center = 0,0,0, cone height = cone radius = 1)
out vec3 tDir;
flat out ivec3 vId;
flat out vec3 vW;
flat out ivec3 lId;
flat out vec3 lW;
void main()
{
    if (selectTree(gPos[0] + cameraRefPos, gParams[0].y) && generate[0] > 0.5 && pass < 2.0) {
        vec3 WCP = getWorldCameraPos();
        vec3 WSD = getWorldSunDir();
        vec3 wPos = (tangentFrameToWorld * vec4(gPos[0], 1.0)).xyz; // world pos
        
        float ca = gParams[0].y * 2.0 - 1.0; // wtf ?
        float sa = sqrt(1.0 - ca * ca);
        mat2 rotation = mat2(ca, sa, -sa, ca); // rotating for what ?
        vec3 tSize = vec3(1.0, 1.0, 0.0) * gParams[0].x * plantRadius;
        
        // local frame
        vec3 gDir = normalize(vec3(gPos[0].xy, 0.0));
        vec3 gLeft = vec3(-gDir.y, gDir.x, 0.0);
        vec3 gUp = vec3(0.0, 0.0, 1.0);
        gLeft *= tSize.x;
        gDir *= tSize.x;
        gUp *= tSize.y;
        
        // tree center
        vec3 O = gPos[0] + gUp * tSize.z / tSize.y + gUp; // TODO tree base expressed in function of tree radius or tree height, or tree half height? must be consistent with usage in brdf model
        bool below = focalPos.z < O.z + gUp.z;
        // quad
        vec3 A = +gLeft - gDir - gUp;
        vec3 B = -gLeft - gDir - gUp;
        vec3 C = +gLeft + (below ? -gDir : gDir) + gUp;
        vec3 D = -gLeft + (below ? -gDir : gDir) + gUp;
        
        // compute camera pos in tree frame
        cPos = (vec3(0.0, 0.0, focalPos.z) - O) / tSize.xxy;
        cPos = vec3(rotation * cPos.xy, cPos.z);
        vec3 cDIR = normalize(cPos);
        lDIR.xyz = normalize(vec3(rotation * tangentSunDir.xy, tangentSunDir.z) / tSize.xxy);
        lDIR.w = length(vec3(rotation * tangentSunDir.xy, tangentSunDir.z) / tSize.xxy);
        
        // compute views for eye and sun
        ivec3 vv;
        vec3 rr;
        findViews(cDIR, vv, rr);
        findViews(lDIR.xyz, lId, lW);
        vId = vv;
        vW = rr;
        
        // project
        gl_Position = tangentFrameToScreen * vec4(O + A, 1.0);
        tDir = O + A - vec3(0.0, 0.0, focalPos.z);
        cDir.xyz = vec3(rotation * A.xy, A.z) / tSize.xxy - cPos;
        cDir.w = dot(cPos + cDir.xyz, cDIR);
        EmitVertex();
        gl_Position = tangentFrameToScreen * vec4(O + B, 1.0);
        tDir = O + B - vec3(0.0, 0.0, focalPos.z);
        cDir.xyz = vec3(rotation * B.xy, B.z) / tSize.xxy - cPos;
        cDir.w = dot(cPos + cDir.xyz, cDIR);
        EmitVertex();
        gl_Position = tangentFrameToScreen * vec4(O + C, 1.0);
        tDir = O + C - vec3(0.0, 0.0, focalPos.z);
        cDir.xyz = vec3(rotation * C.xy, C.z) / tSize.xxy - cPos;
        cDir.w = dot(cPos + cDir.xyz, cDIR);
        EmitVertex();
        gl_Position = tangentFrameToScreen * vec4(O + D, 1.0);
        tDir = O + D - vec3(0.0, 0.0, focalPos.z);
        cDir.xyz = vec3(rotation * D.xy, D.z) / tSize.xxy - cPos;
        cDir.w = dot(cPos + cDir.xyz, cDIR);
        EmitVertex();
        EndPrimitive();
    }
}
#endif




#ifdef _FRAGMENT_
uniform sampler2DArray treeSampler;
uniform Views {
    mat3 views[MAXNT];
};
flat in vec3 cPos;
flat in vec4 lDIR;
in vec4 cDir;
in vec3 tDir;
flat in ivec3 vId;
flat in vec3 vW;
flat in ivec3 lId;
flat in vec3 lW;
layout(location=0) out vec4 color;
void main() {
    vec3 p0 = cPos + cDir.xyz * (1.0 + cDir.w / length(cDir.xyz));
    vec2 uv0 = (views[vId.x] * p0).xy * 0.5 + 0.5;
    vec2 uv1 = (views[vId.y] * p0).xy * 0.5 + 0.5;
    vec2 uv2 = (views[vId.z] * p0).xy * 0.5 + 0.5;
    vec4 c0 = texture(treeSampler, vec3(uv0, vId.x));
    vec4 c1 = texture(treeSampler, vec3(uv1, vId.y));
    vec4 c2 = texture(treeSampler, vec3(uv2, vId.z));
    // linear blend
    color = c0 * vW.x + c1 * vW.y + c2 * vW.z;
    color.rgb /= (color.a == 0.0 ? 1.0 : color.a);
    float t = (color.r * (2.0 * sqrt(2.0)) - sqrt(2.0)) - cDir.w;
    t = 1.0 - t / length(cDir.xyz);
    vec3 p = cPos + cDir.xyz * t;
    // iterative refinement
    uv0 = (views[vId.x] * p).xy * 0.5 + vec2(0.5);
    uv1 = (views[vId.y] * p).xy * 0.5 + vec2(0.5);
    uv2 = (views[vId.z] * p).xy * 0.5 + vec2(0.5);
    c0 = texture(treeSampler, vec3(uv0, vId.x));
    c1 = texture(treeSampler, vec3(uv1, vId.y));
    c2 = texture(treeSampler, vec3(uv2, vId.z));
    vec4 color2 = c0 * vW.x + c1 * vW.y + c2 * vW.z;
    color2.rgb /= (color2.a == 0.0 ? 1.0 : color2.a);
    t = (color2.r * (2.0 * sqrt(2.0)) - sqrt(2.0)) - cDir.w;
    t = 1.0 - t / length(cDir.xyz);
    p = cPos + cDir.xyz * t;
    float fragZ = (gl_FragCoord.z - cameraRefPos.z) / t + cameraRefPos.z;
    gl_FragDepth = fragZ;
    if (color2.a > 0.0) {
        uv0 = (views[lId.x] * p).xy * 0.5 + vec2(0.5);
        uv1 = (views[lId.y] * p).xy * 0.5 + vec2(0.5);
        uv2 = (views[lId.z] * p).xy * 0.5 + vec2(0.5);
        c0 = texture(treeSampler, vec3(uv0, lId.x));
        c1 = texture(treeSampler, vec3(uv1, lId.y));
        c2 = texture(treeSampler, vec3(uv2, lId.z));
        vec4 c = c0 * lW.x + c1 * lW.y + c2 * lW.z;
        c.r /= (c.a == 0.0 ? 1.0 : c.a);
        float d = (c.r * (2.0 * sqrt(2.0)) - sqrt(2.0)) - dot(p, lDIR.xyz);
        float kc = exp(-getTreeTau() * max(d, 0.0));
        vec3 P = vec3(0.0, 0.0, focalPos.z) + tDir * t; // position in tangent frame
        int slice = -1;
        slice += fragZ < getShadowLimit(0) ? 1 : 0;
        slice += fragZ < getShadowLimit(1) ? 1 : 0;
        slice += fragZ < getShadowLimit(2) ? 1 : 0;
        slice += fragZ < getShadowLimit(3) ? 1 : 0;
        if (slice >= 0) {
            mat4 t2s = getTangentFrameToShadow(slice);
            vec3 qs = (t2s * vec4(P, 1.0)).xyz * 0.5 + vec3(0.5);
            float ts = shadow(vec4(qs.xy, slice, qs.z), t2s[3][3]);
            kc *= ts;
        }
        float tz = 0.5 - p.z * 0.5;
        float shear = 2.0 / getTreeHeight();
        float skykc = color2.b / (1.0 + tz * tz * getTreeDensity() / (shear * shear));
        color.rgb = vec3(1.0, pass == 0.0 ? kc : skykc, 0.0);
    }
}
#endif


