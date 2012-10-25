#version 420

uniform mat4 uModelViewProjection;

#ifdef _VERTEX_
layout(location=0) in vec4 iPosition;
layout(location=0) out vec4 oColour;

void main() {
	oColour = iPosition*0.5+0.5;
	gl_Position = uModelViewProjection * iPosition;
}
#endif


#ifdef _FRAGMENT_
layout(location=0) in vec4 iColour;
layout(location=0) out vec4 oColour;

void main() {
	oColour = iColour;
}
#endif
