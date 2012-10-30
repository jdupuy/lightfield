#version 420


//------------------------------------------------------------------------------
#define SQRT_2 1.414213562
#define INV_SQRT_2 0.707106781
#define INV_PI 0.318309886
#define INV_TWO_PI 0.159154943


//------------------------------------------------------------------------------
uniform mat4 uModelView;
uniform mat4 uModelViewProjection;


//------------------------------------------------------------------------------
#ifdef _VERTEX_
layout(location=0) in vec3 iPosition;
layout(location=1) in vec3 iNormal;
layout(location=0) out vec4 oData;

void main() {
	vec4 viewPos = uModelView * vec4(iPosition,1.0);
	gl_Position  = uModelViewProjection * vec4(iPosition,1.0);
	oData.xyz = iNormal;                            // normal in world space
	oData.w   = (-viewPos.z+INV_SQRT_2) * SQRT_2; // depth in view space
}
#endif


//------------------------------------------------------------------------------
#ifdef _FRAGMENT_
layout(location=0) in vec4 iData;
#define iNormal iData.xyz
#define iDepth  iData.w
layout(location=0) out vec4 oData; // in [0,1]

void main() {
#if 1
	vec3 n = normalize(iNormal);
	oData.r = iDepth; // depth 
	oData.g = acos(n.y) * INV_PI; // theta 
	oData.b = fma(atan(n.x,-n.z), INV_TWO_PI, 0.5); // phi
	oData.a = 1.0; // opacity
#else // for debug
	oData.rgb = normalize(iNormal);
	oData.b = -oColour.b;
#endif
}
#endif
