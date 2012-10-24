#version 420

uniform mat4 uModelViewProjection;
uniform int uLayer;

#ifdef _VERTEX_
layout(location=0) in vec4 iPosition;

void main() {
	gl_Position = uModelViewProjection * iPosition;
}
#endif


#ifdef _GEOMETRY_
layout(triangles)in;
layout(max_vertices=3, triangle_strip) out;

void main() {
	gl_Layer = uLayer;

	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();
}
#endif


#ifdef _FRAGMENT_
layout(location=0) out vec4 oColour;

void main() {
	oColour = vec4(1);
}
#endif
