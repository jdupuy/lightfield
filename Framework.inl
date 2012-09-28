////////////////////////////////////////////////////////////////////////////////
// \author J Dupuy
// \brief Utility functions and classes for simple OpenGL demos.
//
////////////////////////////////////////////////////////////////////////////////

namespace fw {
namespace impl {
	class _NoArrayBufferBoundException : public FWException {
	public:
		_NoArrayBufferBoundException() {
			mMessage = "No buffer bound to target GL_ARRAY_BUFFER.";
		}
	};

	class _NoElementArrayBufferBoundException : public FWException {
	public:
		_NoElementArrayBufferBoundException() {
			mMessage = "No buffer bound to target GL_ELEMENT_ARRAY_BUFFER.";
		}
	};

	class _NoDrawIndirectBufferBoundException : public FWException {
	public:
		_NoDrawIndirectBufferBoundException() {
			mMessage = "No buffer bound to target GL_DRAW_INDIRECT_BUFFER.";
		}
	};

	// check bound buffers for mesh generation
	inline void check_bound_buffers() throw(FWException) {
		GLint arrayBuffer = 0;
		GLint elementArrayBuffer = 0;
		GLint drawIndirectBuffer = 0;
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementArrayBuffer);
		glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &drawIndirectBuffer);
		if(arrayBuffer==0)
			throw _NoArrayBufferBoundException();
		if(elementArrayBuffer==0)
			throw _NoElementArrayBufferBoundException();
		if(drawIndirectBuffer==0)
			throw _NoDrawIndirectBufferBoundException();
	}
}

template<class Vertex>
inline
void buffer_cube_data(GLfloat width,
                      GLfloat height,
                      GLfloat depth) throw(FWException) {
	impl::check_bound_buffers();
	const GLint vertexCnt = 24;
	const DrawElementsIndirectCommand command = {24u,0u,0u,0u,0u};
	const GLushort indexes[] = {0u,1u,5u,7u,5u,2u,8u,9u};
	width*= 0.5f;
	height*= 0.5f;
	depth*= 0.5f;
	Vertex *vertices = new Vertex[vertexCnt];
	for(GLint i=0; i<24; ++i)
		vertices[i] = Vertex(width, height, depth,
		                     i%6,i%6,i%6,
		                     i%4,i%4,i%6);

	// Upload data
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(Vertex)*vertexCnt,
	             vertices,
	             GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             sizeof(indexes),
	             indexes,
	             GL_STATIC_DRAW);
	glBufferData(GL_DRAW_INDIRECT_BUFFER,
	             sizeof(DrawElementsIndirectCommand),
	             &command.count,
	             GL_STATIC_DRAW);

	// cleanup
	delete[] vertices;
}

}


