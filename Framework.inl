////////////////////////////////////////////////////////////////////////////////
// \author J Dupuy
// \brief Utility functions and classes for simple OpenGL demos.
//
////////////////////////////////////////////////////////////////////////////////

#include <vector>

namespace fw {
namespace impl {
}

template<class Vertex>
inline
void buffer_cube_data(GLfloat width,
                      GLfloat height,
                      GLfloat depth) throw(FWException) {
	const GLint vertexCnt = 8;
	const GLint indexCnt = 24;
	DrawElementsIndirectCommand command;
	Vertex* vertices  = new Vertex[vertexCnt];
	GLushort* indexes = new GLushort[indexCnt];

	glBufferData(GL_ARRAY_BUFFER, 
	             sizeof(Vertex)*vertexCnt,
	             vertices,
	             GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             sizeof(GLushort)*indexCnt,
	             vertices,
	             GL_STATIC_DRAW);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, 
	             sizeof(DrawElementsIndirectCommand),
	             &command.count,
	             GL_STATIC_DRAW);
}

}


