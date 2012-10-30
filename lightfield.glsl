#version 420

//------------------------------------------------------------------------------
// constants
#define PI 3.141592657
#define TWO_PI 6.283185307
#define SQRT_2 1.414213562
#define INV_SQRT_2 0.707106781


//------------------------------------------------------------------------------
// function decl
void find_views(vec3 cDir,
                out ivec3 layers,
                out vec3 weights);

vec3 spherical_to_cartesian(float theta, float phi);


//------------------------------------------------------------------------------
// uniforms
uniform sampler2DArray sView;
uniform vec3 uViewDir;
uniform int uViewCount;
uniform mat3 uBillboardAxis;
uniform mat4 uModelViewProjection;

layout(std140) uniform ViewAxis {
	mat3 uAxis[VIEWCNT]; // VIEWCNT must be defined
};


//------------------------------------------------------------------------------
// vertex shader
#ifdef _VERTEX_
layout(location=0) out vec3 oTexCoord;

void main() {
	vec2 p = vec2(gl_VertexID & 1, (gl_VertexID >> 1) & 1)*2.0-1.0;
	gl_Position = uModelViewProjection * vec4(p,0,1);
	oTexCoord = uBillboardAxis * vec3(p,0);
}
#endif


//------------------------------------------------------------------------------
// fragment shader
#ifdef _FRAGMENT_
layout(location=0) in  vec3 iTexCoord;
layout(location=0) out vec4 oColour;

void main() {
	ivec3 layers;
	vec3 weights;
	find_views(-uBillboardAxis[2], layers, weights);

	// texcoords for each view
	vec2 texCoord0 = (uAxis[layers[0]] * iTexCoord).st*0.5+0.5;
	vec2 texCoord1 = (uAxis[layers[1]] * iTexCoord).st*0.5+0.5;
	vec2 texCoord2 = (uAxis[layers[2]] * iTexCoord).st*0.5+0.5;
	vec4 t0 = texture(sView, vec3(texCoord0, layers[0]));
	vec4 t1 = texture(sView, vec3(texCoord1, layers[1]));
	vec4 t2 = texture(sView, vec3(texCoord2, layers[2]));
	vec4 t = t0*weights[0]+t1*weights[1]+t2*weights[2]; // lerp

	// second iteration
	vec3 p = - uBillboardAxis[2]*(t.r*SQRT_2-INV_SQRT_2);
	texCoord0 = (uAxis[layers[0]] * p).st*0.5+0.5;
	texCoord1 = (uAxis[layers[1]] * p).st*0.5+0.5;
	texCoord2 = (uAxis[layers[2]] * p).st*0.5+0.5;
	t0 = texture(sView, vec3(texCoord0, layers[0]));
	t1 = texture(sView, vec3(texCoord1, layers[1]));
	t2 = texture(sView, vec3(texCoord2, layers[2]));
	t = t0*weights[0]+t1*weights[1]+t2*weights[2];

	// build output
	oColour.rgb = spherical_to_cartesian(t.g*PI, t.b*TWO_PI)*t.a;
//	oColour.rgb = t.rrr;
}
#endif


//------------------------------------------------------------------------------
// function impl
ivec3 _view_number(ivec3 i, ivec3 j) {
	return i*((2*uViewCount+1)-abs(i))+j+(uViewCount*(uViewCount+1));
}


//------------------------------------------------------------------------------
void find_views(vec3 cDir, out ivec3 layers, out vec3 weights) {
	vec3 VDIR = vec3(cDir.x, max(cDir.y, 0.01), cDir.z);
	float a = abs(VDIR.z) > abs(VDIR.x) ? VDIR.x / VDIR.z : -VDIR.z / VDIR.x;
	float nxx = uViewCount * (1.0 - a) * acos(VDIR.y) / PI;
	float nyy = uViewCount * (1.0 + a) * acos(VDIR.y) / PI;
	int i = int(floor(nxx));
	int j = int(floor(nyy));
	float ti = nxx - i;
	float tj = nyy - j;
	float alpha = 1.0 - ti - tj;
	bool b = alpha > 0.0;
	ivec3 ii = ivec3(b ? i : i + 1, i + 1, i);
	ivec3 jj = ivec3(b ? j : j + 1, j, j + 1);
	weights = vec3(abs(alpha), b ? ti : 1.0 - tj, b ? tj : 1.0 - ti);
	if (abs(VDIR.x) >= abs(VDIR.z)) {
		ivec3 tmp = ii;
		ii = -jj;
		jj = tmp;
	}
	ii *= int(sign(VDIR.x + VDIR.z));
	jj *= int(sign(VDIR.x + VDIR.z));
	layers = _view_number(ii,jj);
}


//------------------------------------------------------------------------------
vec3 spherical_to_cartesian(float theta, float phi) {
	float sinTheta = sin(theta);
	return vec3(sinTheta*cos(phi),
	             cos(theta),
	             sinTheta*sin(phi));
}


