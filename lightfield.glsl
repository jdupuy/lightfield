#version 420

// constants
#define PI 3.141592657


// function decl
void find_views(vec3 cDir,
                out ivec3 layers,
                out vec3 weights);


// uniforms
uniform sampler2DArray sView;
uniform vec3 uViewDir;
uniform int uViewCount;

layout(std140) uniform ViewAxis {
	mat3 uAxis[VIEWCNT]; // VIEWCNT must be defined
}; 

// vertex shader
#ifdef _VERTEX_
layout(location=0) out vec2 oTexCoord;

void main() {
	oTexCoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	gl_Position.xy = oTexCoord*2.0-1.0;
}
#endif


// fragment shader
#ifdef _FRAGMENT_
layout(location=0) in  vec2 iTexCoord;
layout(location=0) out vec4 oColour;

void main() {
	ivec3 layers;
	vec3 weights;
	find_views(uViewDir, layers, weights);

	vec2 texCoord0 = iTexCoord;
	vec2 texCoord1 = iTexCoord;
	vec2 texCoord2 = iTexCoord;

	vec4 t0 = texture(sView, vec3(texCoord0, layers[0]));
	vec4 t1 = texture(sView, vec3(texCoord1, layers[1]));
	vec4 t2 = texture(sView, vec3(texCoord2, layers[2]));

	oColour = t0*weights[0]+t1*weights[1]+t2*weights[2];
}
#endif


// function impl
ivec3 _view_number(ivec3 i, ivec3 j) {
	return i*((2*uViewCount+1)-abs(i))+j+(uViewCount*(uViewCount+1));
}

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


