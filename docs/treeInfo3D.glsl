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

const int MAXN = 9;
const int MAXNT = 2*MAXN*(MAXN+1)+1;

uniform sampler2DArray treeShadowMap;

bool selectTree(vec3 p, float seed)
{
    float d = getTreeDensity();
    if (d == 0.0) {
        return p.x > -10.0 && p.x < 10.0 && p.y > 50.0 && p.y < 70.0 && seed <= 0.01;
    } else {
        return seed <= d;
    }
}
vec3 shadow0(vec3 coord)
{
    return texture(treeShadowMap, coord.xyz).xyz;
}

float shadow1(vec4 coord, float dz)
{
    vec2 za = texture(treeShadowMap, coord.xyz).xy;
    float d = (coord.w - za.x) / dz;
    return d < 0.0 ? 1.0 : za.y;
}

float shadow(vec4 coord, float dz)
{
    float ts = shadow1(coord, dz);

    vec2 off = 1.0 / vec2(textureSize(treeShadowMap, 0).xy);
    ts += shadow1(vec4(coord.x - off.x, coord.y, coord.zw), dz);
    ts += shadow1(vec4(coord.x + off.x, coord.y, coord.zw), dz);
    ts += shadow1(vec4(coord.x, coord.y - off.y, coord.zw), dz);
    ts += shadow1(vec4(coord.x, coord.y + off.y, coord.zw), dz);
    ts += shadow1(vec4(coord.x - off.x, coord.y - off.y, coord.zw), dz);
    ts += shadow1(vec4(coord.x + off.x, coord.y - off.y, coord.zw), dz);
    ts += shadow1(vec4(coord.x - off.x, coord.y + off.y, coord.zw), dz);
    ts += shadow1(vec4(coord.x + off.x, coord.y + off.y, coord.zw), dz);
    ts /= 9.0;

    return ts;
}

int viewNumber(int i, int j) {
    int n = getNViews();
    return i * ((2 * n + 1) - abs(i)) + j + (n * (n + 1));
}

void findViews(vec3 cDIR, out ivec3 vv, out vec3 rr)
{
    int n = getNViews();
    vec3 VDIR = vec3(cDIR.xy, max(cDIR.z, 0.01));
    float a = abs(VDIR.x) > abs(VDIR.y) ? VDIR.y / VDIR.x : -VDIR.x / VDIR.y;
    float nxx = n * (1.0 - a) * acos(VDIR.z) / 3.141592657;
    float nyy = n * (1.0 + a) * acos(VDIR.z) / 3.141592657;
    int i = int(floor(nxx));
    int j = int(floor(nyy));
    float ti = nxx - i;
    float tj = nyy - j;
    float alpha = 1.0 - ti - tj;
    bool b = alpha > 0.0;
    ivec3 ii = ivec3(b ? i : i + 1, i + 1, i);
    ivec3 jj = ivec3(b ? j : j + 1, j, j + 1);
    rr = vec3(abs(alpha), b ? ti : 1.0 - tj, b ? tj : 1.0 - ti);
    if (abs(VDIR.y) >= abs(VDIR.x)) {
        ivec3 tmp = ii;
        ii = -jj;
        jj = tmp;
    }
    ii *= int(sign(VDIR.x + VDIR.y));
    jj *= int(sign(VDIR.x + VDIR.y));
    vv = ivec3(viewNumber(ii.x,jj.x), viewNumber(ii.y,jj.y), viewNumber(ii.z,jj.z));
}
";
