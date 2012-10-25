#version 420

uniform mat4 uModelViewProjection;

#ifdef _VERTEX_
layout(location=0) in vec3 iPosition;
layout(location=1) in vec3 iNormal;
layout(location=0) out vec3 oColour;

void main() {
	oColour = iNormal;
	gl_Position = uModelViewProjection * vec4(iPosition,1.0);
}
#endif


#ifdef _FRAGMENT_
layout(location=0) in vec3 iColour;
layout(location=0) out vec4 oColour;

void main() {
	oColour.rgb = iColour;
}
#endif
