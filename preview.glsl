#version 420

uniform sampler2DArray sView;
uniform float uLayer;

#ifdef _VERTEX_
layout(location=0) out vec2 oTexCoord;

void main() {
	oTexCoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	gl_Position.xy = oTexCoord*2.0-1.0;
}
#endif


#ifdef _FRAGMENT_
layout(location=0)in  vec2 iTexCoord;
layout(location=0)out vec4 oColour;

void main() {
	oColour = texture(sView, vec3(iTexCoord,uLayer));
}

#endif

