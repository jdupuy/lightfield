////////////////////////////////////////////////////////////////////////////////
// \author   Jonathan Dupuy
//
// \TODO second iteration
// \TODO full sphere instead of hemisphere only
// 
////////////////////////////////////////////////////////////////////////////////

// gui
#define _ANT_ENABLE

// GL libraries
#include "glew.hpp"
#include "GL/freeglut.h"

#ifdef _ANT_ENABLE
#	include "AntTweakBar.h"
#endif // _ANT_ENABLE

// Custom libraries
#include "Algebra.hpp"      // Basic algebra library
#include "Transform.hpp"    // Basic transformations
#include "Framework.hpp"    // utility classes/functions
#include "glm.hpp"           // obj loader

// Standard librabries
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <stdexcept>
#include <cmath>


////////////////////////////////////////////////////////////////////////////////
// Global variables
//
////////////////////////////////////////////////////////////////////////////////

// Constants
#define PI 3.14159265
#define SQRT_2 1.414213562

const float FOVY = PI*0.5f;

enum {
	// buffers
	BUFFER_MESH_VERTICES = 0,
	BUFFER_MESH_INDEXES,
	BUFFER_MESH_DRAW,
	BUFFER_LIGHTFIELD_AXIS, // local frame of each view
	BUFFER_COUNT,

	// vertex arrays
	VERTEX_ARRAY_MESH = 0,
	VERTEX_ARRAY_LIGHFIELD,
	VERTEX_ARRAY_COUNT,

	// samplers
	SAMPLER_TRILINEAR = 0,
	SAMPLER_COUNT,

	// textures
	TEXTURE_LIGHFIELD = 0,
	TEXTURE_COUNT,

	// programs
	PROGRAM_MESH = 0,
	PROGRAM_LIGHTFIELD,
	PROGRAM_PREVIEW,
	PROGRAM_COUNT
};

// OpenGL objects
GLuint *buffers      = NULL;
GLuint *vertexArrays = NULL;
GLuint *textures     = NULL;
GLuint *samplers     = NULL;
GLuint *programs     = NULL;

GLsizei lightfieldResolution = 256;
GLsizei viewN = 9;
GLint layer = viewN*(viewN+1);
GLfloat theta = 0.001f; // camera polar angle
GLfloat phi   = 0; // camera azimuthal angle
GLfloat radius     = 2;

bool mouseLeft  = false;
bool mouseRight = false;
GLfloat deltaTicks = 0.0f;

#ifdef _ANT_ENABLE
GLfloat speed = 0.0f; // app speed (in ms)
#endif

////////////////////////////////////////////////////////////////////////////////
// Functions
//
////////////////////////////////////////////////////////////////////////////////

void obj_buffer_data(const std::string& filename) {
	fw::DrawElementsIndirectCommand command;
	GLMmodel *model = glmReadOBJ(filename.c_str());
	if(model == NULL)
		throw(std::runtime_error("failed to load OBJ model"));
	glmUnitize(model); // unit scale
	glmScale(model, 0.5f); // really unit scale

	// GL model
	std::vector<GLfloat>     vertices(model->numvertices*6*2,0.0f);
	std::vector<uint16_t>    indexes(model->numtriangles*3,0);
	std::map<uint32_t, uint16_t>  indexMap;
	vertices.resize(0);
	indexes.resize(0);
	uint16_t nextIndex = 0u;
	// convert to GL batch ready mesh
	for(uint16_t i = 0u; i<model->numtriangles; ++i) {
		uint32_t vertex;
		uint32_t normal;
		std::map<uint32_t, uint16_t>::const_iterator it;

		for(uint8_t j = 0; j < 3u; ++j) {
			vertex = model->triangles[i].vindices[j];
			normal = model->triangles[i].nindices[j];
			it = indexMap.find(vertex + model->numvertices * normal);

			if(it != indexMap.end()) {
				indexes.push_back((*it).second);
			}
			else {
				indexMap[vertex + model->numvertices * normal] = nextIndex;
				indexes.push_back(nextIndex);

				vertices.push_back(model->vertices[vertex*3u]);
				vertices.push_back(model->vertices[vertex*3u+1u]);
				vertices.push_back(model->vertices[vertex*3u+2u]);
		
				vertices.push_back(model->normals[normal*3u]);
				vertices.push_back(model->normals[normal*3u+1u]);
				vertices.push_back(model->normals[normal*3u+2u]);

				++nextIndex;
			}
		}
	}
	glmDelete(model);

	// set indirect drawing command
	command.count = indexes.size();
	command.primCount = 1;
	command.firstIndex = 0;
	command.baseVertex = 0;
	command.baseInstance = 0;

	glBufferData(GL_ARRAY_BUFFER,
		         sizeof(GLfloat)*vertices.size(),
		         &vertices[0],
		         GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		         sizeof(GLushort)*indexes.size(),
		         &indexes[0],
		         GL_STATIC_DRAW);
	glBufferData(GL_DRAW_INDIRECT_BUFFER,
		         sizeof(fw::DrawElementsIndirectCommand),
		         &command,
		         GL_STATIC_DRAW);
}


void load_mesh() {
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_MESH_VERTICES]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_MESH_INDEXES]);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffers[BUFFER_MESH_DRAW]);
		obj_buffer_data("models/Stone_Forest_1.obj");
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void draw_mesh() {
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffers[BUFFER_MESH_DRAW]);
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_MESH]);
	glUseProgram(programs[PROGRAM_MESH]);
		glDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_SHORT,0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}


void build_lighfield() {
	GLuint framebuffer, renderbuffer;
	GLint n = viewN;
	GLint total = 2*n*(n+1)+1;
	GLint current = 0;
	std::vector<Vector4> axis(total*3); // alignment for std140
	axis.resize(0);

	glGenFramebuffers(1, &framebuffer);
	glGenRenderbuffers(1, &renderbuffer);

	glActiveTexture(GL_TEXTURE0+TEXTURE_LIGHFIELD);
	std::vector<GLubyte> pixels(4*total*lightfieldResolution*lightfieldResolution, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_LIGHFIELD]);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 
		             0,
		             GL_RGBA8,
		             lightfieldResolution,
		             lightfieldResolution,
		             total,
		             0,
		             GL_RGBA,
		             GL_UNSIGNED_BYTE,
		             &pixels[0]);
	pixels.clear();

	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER,
	                      GL_DEPTH_COMPONENT24,
	                      lightfieldResolution,
	                      lightfieldResolution);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,
		                          GL_DEPTH_ATTACHMENT,
		                          GL_RENDERBUFFER,
		                          renderbuffer);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// fw::check_framebuffer_status(); should cause error on NVidia
	// also noticed bug on amd when using a FramebufferTexture and a 
	// single depth renderbuffer attachment.

	glViewport(0,0,lightfieldResolution,lightfieldResolution);
	for(GLint i=-n; i<=n; ++i)
		for(GLint j=-n+abs(i);j<=n-abs(i);++j) {
			GLfloat x = (i + j) / float(n);
			GLfloat y = (j - i) / float(n);
			GLfloat angle = (90.0f - std::max(fabs(x),fabs(y)) * 90.0f)*PI/180.0f;
			GLfloat alpha = x == 0.0f && y == 0.0f ? 0.0f 
				: atan2(y, x);

			// compute mvp
			Matrix4x4 rotation = /*Matrix4x4::RotationAboutX(PI*0.5f)
			                   * */Matrix4x4::RotationAboutX(-angle);
			Matrix4x4 mv  = rotation.Inverse()
			              * Matrix4x4::RotationAboutY(-alpha);
			Matrix4x4 mvp = Matrix4x4::Ortho(-SQRT_2*0.5f,
			                                  SQRT_2*0.5f,
			                                 -SQRT_2*0.5f,
			                                  SQRT_2*0.5f,
			                                 -SQRT_2*0.5f,
			                                  SQRT_2*0.5f)
			              * mv;

			// add local frame (transpose of rotation)
			#if 1
			axis.push_back(Vector4(mv[0][0],mv[0][1],mv[0][2],0));
			axis.push_back(Vector4(mv[1][0],mv[1][1],mv[1][2],0));
			axis.push_back(Vector4(mv[2][0],mv[2][1],mv[2][2],0));
			#else
			axis.push_back(Vector4(1,0,0,1));
			axis.push_back(Vector4(0,1,0,1));
			axis.push_back(Vector4(0,0,1,1));
			#endif

			// set uniforms
			glProgramUniform1i(programs[PROGRAM_MESH],
				glGetUniformLocation(programs[PROGRAM_MESH],
				                     "uLayer"),
				                     current);
			glProgramUniformMatrix4fv(programs[PROGRAM_MESH],
				glGetUniformLocation(programs[PROGRAM_MESH],
				                     "uModelView"),
				                     1, 
				                     GL_FALSE,
				                     reinterpret_cast<const GLfloat*>
				                     (&mv));
			glProgramUniformMatrix4fv(programs[PROGRAM_MESH],
				glGetUniformLocation(programs[PROGRAM_MESH],
				                     "uModelViewProjection"),
				                     1, 
				                     GL_FALSE,
				                     reinterpret_cast<const GLfloat*>
				                     (&mvp));

			glFramebufferTextureLayer(GL_FRAMEBUFFER,
		                              GL_COLOR_ATTACHMENT0,
		                              textures[TEXTURE_LIGHFIELD],
		                              0,
		                              current);
	fw::check_framebuffer_status();

			glClear(GL_DEPTH_BUFFER_BIT);
			draw_mesh();

			++current;
		}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	// upload matrices
	glBindBuffer(GL_UNIFORM_BUFFER, buffers[BUFFER_LIGHTFIELD_AXIS]);
		glBufferData(GL_UNIFORM_BUFFER,
		             sizeof(Vector4)*axis.size(),
		             &axis[0],
		             GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glDeleteFramebuffers(1, &framebuffer);
	glDeleteRenderbuffers(1, &renderbuffer);
}


#ifdef _ANT_ENABLE

static void TW_CALL toggle_fullscreen(void *data) {
	glutFullScreenToggle();
}

#endif

////////////////////////////////////////////////////////////////////////////////
// on init cb
void on_init() {
	// alloc names
	buffers      = new GLuint[BUFFER_COUNT];
	vertexArrays = new GLuint[VERTEX_ARRAY_COUNT];
	textures     = new GLuint[TEXTURE_COUNT];
	samplers     = new GLuint[SAMPLER_COUNT];
	programs     = new GLuint[PROGRAM_COUNT];

	fw::init_debug_output(std::cout);

	// gen names
	glGenBuffers(BUFFER_COUNT, buffers);
	glGenVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	glGenTextures(TEXTURE_COUNT, textures);
	glGenSamplers(SAMPLER_COUNT, samplers);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		programs[i] = glCreateProgram();

	glEnable(GL_DEPTH_TEST);

	// build programs
	fw::build_glsl_program(programs[PROGRAM_MESH],
	                       "mesh.glsl",
	                       "",
	                       GL_TRUE);
	fw::build_glsl_program(programs[PROGRAM_PREVIEW],
	                       "preview.glsl",
	                       "",
	                       GL_TRUE);
	fw::build_glsl_program(programs[PROGRAM_LIGHTFIELD],
	                       "lightfield.glsl",
	                       "#define VIEWCNT 512",
	                       GL_TRUE);

	glUniformBlockBinding(programs[PROGRAM_LIGHTFIELD],
	                      glGetUniformBlockIndex(programs[PROGRAM_LIGHTFIELD],
	                                             "ViewAxis"),
	                      BUFFER_LIGHTFIELD_AXIS);

	glProgramUniform1i(programs[PROGRAM_PREVIEW],
		glGetUniformLocation(programs[PROGRAM_PREVIEW],
		                     "sView"),
		               TEXTURE_LIGHFIELD);
	glProgramUniform1i(programs[PROGRAM_LIGHTFIELD],
		glGetUniformLocation(programs[PROGRAM_LIGHTFIELD],
		                     "sView"),
		               TEXTURE_LIGHFIELD);
	glProgramUniform1i(programs[PROGRAM_LIGHTFIELD],
		glGetUniformLocation(programs[PROGRAM_LIGHTFIELD],
		                     "uViewCount"),
		               viewN);

	// vertex arrays
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_MESH]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_MESH_INDEXES]);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_MESH_VERTICES]);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0,3,GL_FLOAT,0,24,0);
		glVertexAttribPointer(1,3,GL_FLOAT,0,24,FW_BUFFER_OFFSET(12));
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_LIGHFIELD]);
	glBindVertexArray(0);

	load_mesh();
	build_lighfield();

	glBindBufferBase(GL_UNIFORM_BUFFER,
	                 BUFFER_LIGHTFIELD_AXIS,
	                 buffers[BUFFER_LIGHTFIELD_AXIS]);

	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
	                    GL_TEXTURE_WRAP_S,
	                    GL_CLAMP_TO_BORDER);
	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
	                    GL_TEXTURE_WRAP_T,
	                    GL_CLAMP_TO_BORDER);
//	const GLfloat borderColour[]={1,0,0,1};
//	glSamplerParameterfv(samplers[SAMPLER_TRILINEAR],
//	                     GL_TEXTURE_BORDER_COLOR,
//	                     borderColour);

	glBindSampler(TEXTURE_LIGHFIELD, samplers[SAMPLER_TRILINEAR]);

#ifdef _ANT_ENABLE
	// start ant
	TwInit(TW_OPENGL_CORE, NULL);
	// send the ''glutGetModifers'' function pointer to AntTweakBar
	TwGLUTModifiersFunc(glutGetModifiers);

	// Create a new bar
	TwBar* menuBar = TwNewBar("menu");
	TwDefine("menu size='200 170'");
	TwDefine("menu position='0 0'");
	TwDefine("menu alpha='255'");
	TwDefine("menu valueswidth=85");

	TwAddVarRO(menuBar,
	           "speed (ms)",
	           TW_TYPE_FLOAT,
	           &speed,
	           "");

	TwAddButton( menuBar,
	             "fullscreen",
	             &toggle_fullscreen,
	             NULL,
	             "label='toggle fullscreen'");

	TwAddVarRW(menuBar,
	           "layer",
	           TW_TYPE_INT32,
	           &layer,
	           "min=0 max=999");

	TwAddVarRW(menuBar,
	           "theta",
	           TW_TYPE_FLOAT,
	           &theta,
	           "min=0.001 max=90 step=1");

	TwAddVarRW(menuBar,
	           "phi",
	           TW_TYPE_FLOAT,
	           &phi,
	           "min=0 max=360 step=1");

#endif // _ANT_ENABLE
	fw::check_gl_error();
}


////////////////////////////////////////////////////////////////////////////////
// on clean cb
void on_clean() {
	// delete objects
	glDeleteBuffers(BUFFER_COUNT, buffers);
	glDeleteVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	glDeleteTextures(TEXTURE_COUNT, textures);
	glDeleteSamplers(SAMPLER_COUNT, samplers);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		glDeleteProgram(programs[i]);

	// release memory
	delete[] buffers;
	delete[] vertexArrays;
	delete[] textures;
	delete[] samplers;
	delete[] programs;

#ifdef _ANT_ENABLE
	TwTerminate();
#endif // _ANT_ENABLE

	fw::check_gl_error();
}


////////////////////////////////////////////////////////////////////////////////
// on update cb
void on_update() {
	// Variables
	static fw::Timer deltaTimer;

	// stop timing and set delta
	deltaTimer.Stop();
	deltaTicks = deltaTimer.Ticks();
#ifdef _ANT_ENABLE
	speed = deltaTicks*1000.0f;
#endif

	glProgramUniform1f(programs[PROGRAM_PREVIEW],
		glGetUniformLocation(programs[PROGRAM_PREVIEW],
		                     "uLayer"),
		               layer);

	float thetaR = PI*0.5f-theta * PI / 180.0f;
	float phiR   = phi   * PI / 180.0f;
//	float cosTheta = cos(thetaR);
//	float cosPhi   = cos(phiR);
//	float sinTheta = sin(thetaR);
//	float sinPhi   = sin(phiR);
	Affine objectAxis;
	objectAxis.TranslateWorld(Vector3(0,0,-radius));

	Matrix4x4 mvp = Matrix4x4::Perspective(FOVY,1,0.05f,1000.0f)
	              * objectAxis.ExtractTransformMatrix();

	objectAxis.RotateAboutWorldX(thetaR);
	objectAxis.RotateAboutWorldY(phiR);

	Vector3 camPos = objectAxis.GetUnitAxis() * objectAxis.GetPosition();

	glProgramUniform3f(programs[PROGRAM_LIGHTFIELD],
		glGetUniformLocation(programs[PROGRAM_LIGHTFIELD],
		                     "uCamPos"),
		               camPos[0],camPos[1],camPos[2]);
	glProgramUniformMatrix3fv(programs[PROGRAM_LIGHTFIELD],
		glGetUniformLocation(programs[PROGRAM_LIGHTFIELD],
		                     "uBillboardAxis"),
		                      1,
		                      GL_FALSE,
		                      &objectAxis.GetUnitAxis()[0][0]);
	glProgramUniformMatrix4fv(programs[PROGRAM_LIGHTFIELD],
		glGetUniformLocation(programs[PROGRAM_LIGHTFIELD],
		                     "uModelViewProjection"),
		                      1,
		                      GL_FALSE,
		                      &mvp[0][0]);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(200,lightfieldResolution,lightfieldResolution, lightfieldResolution);

	glUseProgram(programs[PROGRAM_LIGHTFIELD]);
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_LIGHFIELD]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glViewport(200,0,lightfieldResolution, lightfieldResolution);
	glUseProgram(programs[PROGRAM_PREVIEW]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// start ticking
	deltaTimer.Start();

#ifdef _ANT_ENABLE
	glBindSampler(TEXTURE_LIGHFIELD, 0);
	TwDraw();
	glBindSampler(TEXTURE_LIGHFIELD, samplers[SAMPLER_TRILINEAR]);
#endif // _ANT_ENABLE

	fw::check_gl_error();

	glutSwapBuffers();
	glutPostRedisplay();
}


////////////////////////////////////////////////////////////////////////////////
// on resize cb
void on_resize(GLint w, GLint h) {
#ifdef _ANT_ENABLE
	TwWindowSize(w, h);
#endif
}


////////////////////////////////////////////////////////////////////////////////
// on key down cb
void on_key_down(GLubyte key, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1==TwEventKeyboardGLUT(key, x, y))
		return;
#endif
	if (key==27) // escape
		glutLeaveMainLoop();
	if(key=='f')
		glutFullScreenToggle();
	if(key=='p')
		fw::save_gl_front_buffer(0,
		                         0,
		                         glutGet(GLUT_WINDOW_WIDTH),
		                         glutGet(GLUT_WINDOW_HEIGHT));

}


////////////////////////////////////////////////////////////////////////////////
// on mouse button cb
void on_mouse_button(GLint button, GLint state, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwEventMouseButtonGLUT(button, state, x, y))
		return;
#endif // _ANT_ENABLE
	if(state==GLUT_DOWN) {
		mouseLeft  |= button == GLUT_LEFT_BUTTON;
		mouseRight |= button == GLUT_RIGHT_BUTTON;
	}
	else {
		mouseLeft  &= button == GLUT_LEFT_BUTTON ? false : mouseLeft;
		mouseRight  &= button == GLUT_RIGHT_BUTTON ? false : mouseRight;
	}
	if(button == 4) {
		radius += 0.5f;
	}
	if(button == 3) {
		radius = std::max(0.5f, radius-0.5f);
	}
}


////////////////////////////////////////////////////////////////////////////////
// on mouse motion cb
void on_mouse_motion(GLint x, GLint y) {
#ifdef _ANT_ENABLE
	TwEventMouseMotionGLUT(x,y);
#endif // _ANT_ENABLE

	static GLint sMousePreviousX = 0;
	static GLint sMousePreviousY = 0;
	const GLint MOUSE_XREL = x-sMousePreviousX;
	const GLint MOUSE_YREL = y-sMousePreviousY;
	sMousePreviousX = x;
	sMousePreviousY = y;

	if(mouseLeft) {
		phi   = fmod(phi+deltaTicks*MOUSE_XREL*400.0f, 360.0f);
		theta = std::min(std::max(0.001f,theta-deltaTicks*MOUSE_YREL*400.0f), 90.0f);
	}
}


////////////////////////////////////////////////////////////////////////////////
// on mouse wheel cb
void on_mouse_wheel(GLint wheel, GLint direction, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwMouseWheel(wheel))
		return;
#endif // _ANT_ENABLE
}


////////////////////////////////////////////////////////////////////////////////
// Main
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	const GLuint CONTEXT_MAJOR = 4;
	const GLuint CONTEXT_MINOR = 2;

	// init glut
	glutInit(&argc, argv);
	glutInitContextVersion(CONTEXT_MAJOR ,CONTEXT_MINOR);
	glutInitContextFlags(GLUT_DEBUG | GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);


	// build window
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(lightfieldResolution+300, lightfieldResolution*2);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("OpenGL");

	// init glew
	glewExperimental = GL_TRUE; // segfault on GenVertexArrays on Nvidia otherwise
	GLenum err = glewInit();
	if(GLEW_OK != err) {
		std::stringstream ss;
		ss << err;
		std::cerr << "glewInit() gave error " << ss.str() << std::endl;
		return 1;
	}

	// glewInit generates an INVALID_ENUM error for some reason...
	glGetError();

	// set callbacks
	glutCloseFunc(&on_clean);
	glutReshapeFunc(&on_resize);
	glutDisplayFunc(&on_update);
	glutKeyboardFunc(&on_key_down);
	glutMouseFunc(&on_mouse_button);
	glutPassiveMotionFunc(&on_mouse_motion);
	glutMotionFunc(&on_mouse_motion);
	glutMouseWheelFunc(&on_mouse_wheel);

	// run
	try {
		// run demo
		on_init();
		glutMainLoop();
	}
	catch(std::exception& e) {
		std::cerr << "Fatal exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

