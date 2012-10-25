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

uniform globals {
    vec3 worldCameraPos;
    vec3 worldSunDir;
    float minRadius;
    float maxRadius;
    float treeTau;
    float treeHeight;
    float treeDensity;
    int nViews;
    float maxTreeDistance;
    vec4 shadowLimit;
    mat4 tangentFrameToShadow[4];
};

vec3 getWorldCameraPos() {
    return worldCameraPos;
}

vec3 getWorldSunDir() {
    return worldSunDir;
}

float getMinRadius() {
    return minRadius;
}

float getMaxRadius() {
    return maxRadius;
}

float getTreeTau() {
    return treeTau;
}

float getTreeHeight() {
    return treeHeight;
}

float getTreeDensity() {
    return treeDensity;
}

int getNViews() {
    return nViews;
}

float getMaxTreeDistance() {
    return maxTreeDistance;
}

float getShadowLimit(int i)
{
    return shadowLimit[i];
}

mat4 getTangentFrameToShadow(int i)
{
    return tangentFrameToShadow[i];
}
";
