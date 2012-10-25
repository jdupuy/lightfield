////////////////////////////////////////////////////////////////////////////////
// \author   Jonathan Dupuy
//
////////////////////////////////////////////////////////////////////////////////

#include "Framework.hpp"

#include <fstream> // std::ifstream
#include <climits> // CHAR_BIT
#include <cstring> // memcpy
#include <sstream> // std::stringstream
#include <algorithm> // std::min std::max

#ifdef _WIN32
#	define NOMINMAX
#	include <windows.h>
#	include <winbase.h>
#else
#	include <sys/time.h>
#endif // _WIN32

#ifndef _NO_PNG
#	include "png.h"
#endif

namespace fw {
////////////////////////////////////////////////////////////////////////////////
// Exceptions
//
////////////////////////////////////////////////////////////////////////////////
class _ShaderCompilationFailedException : public FWException {
public:
	_ShaderCompilationFailedException(const std::string& log) {
		mMessage = log;
	}
};

class _ProgramLinkFailException : public FWException {
public:
	_ProgramLinkFailException(const std::string& file, const std::string& log) {
		mMessage = "GLSL link error in " + file + ":\n" + log;
	}
};

class _ProgramBuildFailException : public FWException {
public:
	_ProgramBuildFailException(const std::string& file, const std::string& log) {
		mMessage = "GLSL build error in " + file + ":\n" + log;
	}
};

class _FileNotFoundException : public FWException {
public:
	_FileNotFoundException(const std::string& file) {
		mMessage = "File " + file + " not found.";
	}
};

class _ProgramInvalidFirstLineException : public FWException {
public:
	_ProgramInvalidFirstLineException(const std::string& file) {
		mMessage = "First line must be GLSL version specification (in "+file+").";
	}
};

class _GLErrorException : public FWException {
public:
	_GLErrorException(const std::string& log) {
		mMessage = log;
	}
};

class _GLFramebufferStatusException : public FWException {
public:
	_GLFramebufferStatusException(const std::string& log) {
		mMessage = log;
	}
};

class _InvalidViewportDimensionsException : public FWException {
public:
	_InvalidViewportDimensionsException() {
		mMessage = "Invalid viewport dimensions.";
	}
};

class _InvalidPerspectiveException : public FWException {
public:
	_InvalidPerspectiveException() {
		mMessage = "Invalid perspective projections parameters.";
	}
};

class _InvalidOrthoException : public FWException {
public:
	_InvalidOrthoException() {
		mMessage = "Invalid ortho projection parameters.";
	}
};

class _DebugOutputNotSupportedException : public FWException {
public:
	_DebugOutputNotSupportedException() {
		mMessage = "Platform does not support ARB_debug_output.";
	}
};

class _DebugOutputAlreadyConfiguredException : public FWException {
public:
	_DebugOutputAlreadyConfiguredException() {
		mMessage = "Debug Output can only be set once.";
	}
};

class _ImmutableTexturesNotSupportedException : public FWException {
public:
	_ImmutableTexturesNotSupportedException() {
		mMessage = "Platform does not support ARB_texture_storage.";
	}
};

class _NullParamException : public FWException {
public:
	_NullParamException() {
		mMessage = "Parameter passed as NULL.";
	}
};

class _MemoryAllocationFailedException : public FWException {
public:
	_MemoryAllocationFailedException() {
		mMessage = "Memory allocation failed.";
	}
};

#ifndef _NO_PNG
class _PngInvalidHeaderException : public FWException {
public:
	_PngInvalidHeaderException(const std::string& file) {
		mMessage = "File " + file + " has invalid PNG header.";
	}
};

class _PngCreateReadStructFailedException : public FWException {
public:
	_PngCreateReadStructFailedException(const std::string& file) {
		mMessage = "Libpng failed to create read struct for file " + file + '.';
	}
};

class _PngUnsupportedBitDepthException : public FWException {
public:
	_PngUnsupportedBitDepthException(const std::string& file) {
		mMessage = "File " + file + " has unsupported bit depth.";
	}
};

class _PngSetJmpFailedException : public FWException {
public:
	_PngSetJmpFailedException(const std::string& file) {
		mMessage = "Libpng failed to set jmp for file " + file + '.';
	}
};

class _PngCreateInfoStructFailedException : public FWException {
public:
	_PngCreateInfoStructFailedException(const std::string& file) {
		mMessage = "Libpng faile to create info struct for file " 
		         + file + '.';
	}
};

class _PngUnsupportedColourTypeException : public FWException {
public:
	_PngUnsupportedColourTypeException(const std::string& file) {
		mMessage = "Png file " + file + "has unsupported colour type.";
	}
};

#endif


////////////////////////////////////////////////////////////////////////////////
// Local functions
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Get time
static GLdouble _get_ticks() {
#ifdef _WIN32
	__int64 time;
	__int64 cpuFrequency;
	QueryPerformanceCounter((LARGE_INTEGER*) &time);
	QueryPerformanceFrequency((LARGE_INTEGER*) &cpuFrequency);
	return time / static_cast<GLdouble>(cpuFrequency);
#else
	static GLdouble t0 = 0.0;
	timeval tv;
	gettimeofday(&tv, 0);
	if (!t0) 
		t0 = tv.tv_sec;

	return   static_cast<GLdouble>(tv.tv_sec-t0)
	       + static_cast<GLdouble>(tv.tv_usec) / 1e6;
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Attach shader
static void _attach_shader(GLuint program,
                           GLenum shaderType,
                           const GLchar* stringsrc) throw(FWException) {
	const GLchar** string = &stringsrc;
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, string, NULL);
	glCompileShader(shader);

	// check compilation
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE)
	{
		GLchar logContent[256];
		glGetShaderInfoLog(shader, 256, NULL, logContent);
		throw _ShaderCompilationFailedException(std::string(logContent));
	}

	// attach shader and flag for deletion
	glAttachShader(program, shader);
	glDeleteShader(shader);
}


////////////////////////////////////////////////////////////////////////////////
// Convert GL error code to string
static const std::string _gl_error_to_string(GLenum error) {
	switch(error) {
	case GL_NO_ERROR:
		return "GL_NO_ERROR";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	default:
		return "unknown code";
	}
}


////////////////////////////////////////////////////////////////////////////////
// Convert GL framebuffer status code to string
static const std::string _gl_framebuffer_status_to_string(GLenum error) {
	switch(error) {
	case GL_FRAMEBUFFER_COMPLETE:
		return "GL_FRAMEBUFFER_COMPLETE";
	case GL_FRAMEBUFFER_UNDEFINED:
		return "GL_FRAMEBUFFER_UNDEFINED";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "GL_FRAMEBUFFER_UNSUPPORTED";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
	default:
		return "unknown code";
	}
}


////////////////////////////////////////////////////////////////////////////////
// OpenGL Debug ARB callback
static GLvoid _gl_debug_message_callback( GLenum source,
                                          GLenum type,
                                          GLuint id,
                                          GLenum severity,
                                          GLsizei length,
                                          const GLchar* message,
                                          GLvoid* userParam ) {
    std::ofstream& outputStream = *reinterpret_cast<std::ofstream*>(userParam);
	outputStream << "[DEBUG_OUTPUT] " << message << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
// create a frustum matrix
#define left   frustum[0]
#define right  frustum[1]
#define bottom frustum[2]
#define top    frustum[3]
#define near   frustum[4]
#define far    frustum[5]

static void _perspective_matrix(const GLfloat *frustum, 
                                GLfloat *matrix) throw(FWException) {
	if(left == right || bottom == top || near > far || near < 0.0f)
		throw _InvalidPerspectiveException();
	float oneOverRightMinusLeft = 1.0f/(right - left);
	float oneOverTopMinusBottom = 1.0f/(top   - bottom);
	float oneOverFarMinusNear   = 1.0f/(far   - near);
	float twoNearVal            = 2.0f * near;
	matrix[0]  = twoNearVal * oneOverRightMinusLeft;
	matrix[5]  = twoNearVal * oneOverTopMinusBottom;
	matrix[8]  = (right + left) * oneOverRightMinusLeft;
	matrix[9]  = (top + bottom) * oneOverTopMinusBottom;
	matrix[10] = -(far + near) * oneOverFarMinusNear;
	matrix[11] = -1.0f;
	matrix[14] = -(twoNearVal*far) * oneOverFarMinusNear;
}

static void _ortho_matrix(const GLfloat *frustum, 
                          GLfloat *matrix) throw(FWException) {
	if(left == right || bottom == top || near == far)
		throw _InvalidOrthoException();
	float oneOverRightMinusLeft = 1.0f/(right - left);
	float oneOverTopMinusBottom = 1.0f/(top - bottom);
	float oneOverFarMinusNear   = 1.0f/(far - near);
	matrix[0]  = 2.0f * oneOverRightMinusLeft;
	matrix[5]  = 2.0f * oneOverTopMinusBottom;
	matrix[10] = -2.0f * oneOverFarMinusNear;
	matrix[12] = -(right + left) * oneOverRightMinusLeft;
	matrix[13] = -(top + bottom) * oneOverTopMinusBottom;
	matrix[14] = -(far + near) * oneOverFarMinusNear;
	matrix[15] = 1.0f;
} 

#undef left
#undef right
#undef bottom
#undef top
#undef near
#undef far

static void _frustum_matrix(const GLfloat *frustum, 
                            bool perspective,
                            GLfloat *matrix) {
	matrix[1]=matrix[2]
	         =matrix[3]
	         =matrix[4]
	         =matrix[6]
	         =matrix[7]
	         =0.f;
	if(perspective)
		_perspective_matrix(frustum, matrix);
	else
		_ortho_matrix(frustum, matrix);
} 


////////////////////////////////////////////////////////////////////////////////
// save buffer
static GLvoid _save_gl_buffer(GLint x,
	                          GLint y,
	                          GLsizei width,
	                          GLsizei height,
	                          GLenum buffer) throw(FWException) {
	static GLuint sShotCounter = 1;
	GLubyte *tgaPixels = NULL;
	GLint tgaWidth, tgaHeight;
	GLint readFramebuffer,
	      readBuffer,
	      pixelPackBufferBinding,
	      packSwapBytes, packLsbFirst, packRowLength, packImageHeight,
	      packSkipRows, packSkipPixels, packSkipImages, packAlignment;

	// check values
	if(x>=width || y>=height || x<0 || y<0)
		throw _InvalidViewportDimensionsException();

	// compute final tga dimensions
	tgaWidth  = width  - x;
	tgaHeight = height - y;

	// save GL state
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
	glGetIntegerv(GL_READ_BUFFER, &readBuffer);
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pixelPackBufferBinding);
	glGetIntegerv(GL_PACK_SWAP_BYTES, &packSwapBytes);
	glGetIntegerv(GL_PACK_LSB_FIRST, &packLsbFirst);
	glGetIntegerv(GL_PACK_ROW_LENGTH, &packRowLength);
	glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &packImageHeight);
	glGetIntegerv(GL_PACK_SKIP_ROWS, &packSkipRows);
	glGetIntegerv(GL_PACK_SKIP_PIXELS, &packSkipPixels);
	glGetIntegerv(GL_PACK_SKIP_IMAGES, &packSkipImages);
	glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);

	// push gl state
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadBuffer(buffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glPixelStorei(GL_PACK_SWAP_BYTES, 0);
	glPixelStorei(GL_PACK_LSB_FIRST, 0);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_PACK_SKIP_IMAGES, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	// allocate data and read mPixels frome framebuffer
	tgaPixels = new GLubyte[tgaWidth*tgaHeight*3];
	glReadPixels(x, y, width, height, GL_BGR, GL_UNSIGNED_BYTE, tgaPixels);

	// compute new filename
	std::stringstream ss;
	ss << "screenshot";
	if(sShotCounter < 10)
		ss << '0';
	if(sShotCounter < 100)
		ss << '0';
	ss << sShotCounter << ".tga";

	// open file
	std::ofstream fileStream( ss.str().c_str(),
	                          std::ifstream::out | std::ifstream::binary );
	// check opening
	if(!fileStream)
		throw _FileNotFoundException(ss.str().c_str());

	// create header
	GLchar tgaHeader[18]= {
		0,                                     // image identification field
		0,                                     // colormap type
		2,                                     // image type code
		0,0,0,0,0,                             // color map spec (ignored here)
		0,0,                                   // x origin of image
		0,0,                                   // y origin of image
		tgaWidth & 255,  tgaWidth >> 8 & 255,  // width of the image
		tgaHeight & 255, tgaHeight >> 8 & 255, // height of the image
		24,                                    // bits per pixel
		0                                      // image descriptor byte
	};

	// write header and pixel data
	fileStream.write(tgaHeader, 18);
	fileStream.write(reinterpret_cast<const GLchar*>(tgaPixels),
	                 tgaWidth*tgaHeight*3);
	fileStream.close();

	// restore GL state
	glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
	glReadBuffer(readBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelPackBufferBinding);
	glPixelStorei(GL_PACK_SWAP_BYTES, packSwapBytes);
	glPixelStorei(GL_PACK_LSB_FIRST, packLsbFirst);
	glPixelStorei(GL_PACK_ROW_LENGTH, packRowLength);
	glPixelStorei(GL_PACK_IMAGE_HEIGHT, packImageHeight);
	glPixelStorei(GL_PACK_SKIP_ROWS, packSkipRows);
	glPixelStorei(GL_PACK_SKIP_PIXELS, packSkipPixels);
	glPixelStorei(GL_PACK_SKIP_IMAGES, packSkipImages);
	glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);

	// free memory
	delete[] tgaPixels;

	// increment screenshot counter
	++sShotCounter;
}


////////////////////////////////////////////////////////////////////////////////
// Generic texture uploads
template <typename IMG_T>
void tex_img_image2D(const std::string& filename,
                     GLboolean genMipmaps,
                     GLboolean immutable,
                     void (*extract_format_func)
                     (const IMG_T&, GLenum&, GLenum&)) throw(FWException) {
	if(!GLEW_ARB_texture_storage && immutable)
		throw _ImmutableTexturesNotSupportedException();

	IMG_T img(filename);
	GLsizei size = std::max(img.Width(), img.Height());
	GLint levels = !genMipmaps ? 1 : next_power_of_two_exponent(size);
	GLenum internalFormat, pixelFormat;
	extract_format_func(img, internalFormat, pixelFormat);

	// allocate memory
	if(immutable)
		glTexStorage2D(GL_TEXTURE_2D,
		               levels,
		               internalFormat,
		               img.Width(),
		               img.Height());
	else
		glTexImage2D(GL_TEXTURE_2D,
		             0,
		             internalFormat,
		             img.Width(),
		             img.Height(),
		             0,
		             pixelFormat,
		             GL_UNSIGNED_BYTE,
		             NULL);

	// send data
	GLint align(0), swapBytes(0);
	GLenum pixelData = GL_UNSIGNED_BYTE;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
	glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapBytes);
	if(img.BitsPerPixel()==16) {
		glPixelStorei(GL_UNPACK_ALIGNMENT,2);
		glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_TRUE);
		pixelData = GL_UNSIGNED_SHORT;
	}
	else {
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_FALSE);
	}
	glTexSubImage2D(GL_TEXTURE_2D,
	                0,
	                0, 0,
	                img.Width(), img.Height(),
	                pixelFormat,
	                pixelData,
	                img.Pixels());
	glPixelStorei(GL_UNPACK_ALIGNMENT,align);
	glPixelStorei(GL_UNPACK_SWAP_BYTES,swapBytes);

	if(genMipmaps == GL_TRUE)
		glGenerateMipmap(GL_TEXTURE_2D);
}

template<typename IMG_T>
void tex_img_cube_map(const std::string filenames[6],
                      GLboolean genMipmaps,
                      GLboolean immutable,
                      void (*extract_format_func)
                      (const IMG_T&, GLenum&, GLenum&)) throw(FWException) {
	if(!GLEW_ARB_texture_storage && immutable)
		throw _ImmutableTexturesNotSupportedException();

	// Load images
	IMG_T xpos(filenames[0]);
	IMG_T xneg(filenames[1]);
	IMG_T ypos(filenames[2]);
	IMG_T yneg(filenames[3]);
	IMG_T zpos(filenames[4]);
	IMG_T zneg(filenames[5]);
	/// TODO check consistency

	GLsizei size = std::max(xpos.Width(), xpos.Height());
	GLint levels = !genMipmaps ? 1 : next_power_of_two_exponent(size);
	GLenum internalFormat, pixelFormat;
	extract_format_func(xpos, internalFormat, pixelFormat);

	// allocate memory
	if(immutable)
		glTexStorage2D(GL_TEXTURE_CUBE_MAP,
		               levels,
		               internalFormat,
		               xpos.Width(),
		               xpos.Height());
	else {
		for(GLint i=0; i<6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X+i,
			             0,
			             internalFormat,
			             xpos.Width(),
			             xpos.Height(),
			             0,
			             pixelFormat,
			             GL_UNSIGNED_BYTE,
			             NULL);
	}

	// send data
	const GLvoid* dataPtr[6] = {
	xpos.Pixels(), xneg.Pixels(),
	ypos.Pixels(), yneg.Pixels(),
	zpos.Pixels(), zneg.Pixels() };
	GLint align(0), swapBytes(0);

	GLenum pixelData = GL_UNSIGNED_BYTE;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
	glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapBytes);
	if(xpos.BitsPerPixel()==16) {
		glPixelStorei(GL_UNPACK_ALIGNMENT,2);
		glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_TRUE);
		pixelData = GL_UNSIGNED_SHORT;
	}
	else {
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_FALSE);
	}
	for(GLint i=0; i<6; ++i)
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,
		                0,
		                0, 0,
		                xpos.Width(), xpos.Height(),
		                pixelFormat,
		                pixelData,
		                dataPtr[i]);
	glPixelStorei(GL_UNPACK_ALIGNMENT,align);
	glPixelStorei(GL_UNPACK_SWAP_BYTES,swapBytes);

	if(genMipmaps == GL_TRUE)
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

template<typename IMG_T>
void tex_img_sprites_image3D(const std::vector<std::string>& filenames,
	                         GLboolean genMipmaps,
	                         GLboolean immutable,
	                         void (*extract_format_func)
	                         (const IMG_T&, GLenum&, GLenum&)) throw(FWException) {
	if(!GLEW_ARB_texture_storage && immutable)
		throw _ImmutableTexturesNotSupportedException();

	// Load images
	GLsizei frameCnt = (GLsizei)filenames.size();
	IMG_T *imgs = new IMG_T[frameCnt];
	for(GLsizei i=0; i<frameCnt; ++i) {
		imgs[i].Load(filenames[i]);
	}

	GLsizei size = std::max(GLsizei(std::max(imgs[0].Width(),
	                                         imgs[0].Height())),
	                        frameCnt);
	GLint levels = !genMipmaps ? 1 : next_power_of_two_exponent(size);
	GLenum internalFormat, pixelFormat;
	extract_format_func(imgs[0], internalFormat, pixelFormat);

	// allocate memory
	if(immutable)
		glTexStorage3D(GL_TEXTURE_3D,
		               levels,
		               internalFormat,
		               imgs[0].Width(),
		               imgs[0].Height(),
		               frameCnt);
	else 
		glTexImage3D(GL_TEXTURE_3D,
		             0,
		             internalFormat,
		             imgs[0].Width(),
		             imgs[0].Height(),
		             frameCnt,
		             0,
		             pixelFormat,
		             GL_UNSIGNED_BYTE,
		             NULL);

	// send data
	GLint align(0), swapBytes(0);
	GLenum pixelData = GL_UNSIGNED_BYTE;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
	glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapBytes);
	if(imgs[0].BitsPerPixel()==16) {
		glPixelStorei(GL_UNPACK_ALIGNMENT,2);
		glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_TRUE);
		pixelData = GL_UNSIGNED_SHORT;
	}
	else {
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_FALSE);
	}
	for(GLint i=0; i<frameCnt; ++i)
		glTexSubImage3D(GL_TEXTURE_3D,
		                0,
		                0, 0, i,
		                imgs[i].Width(), imgs[i].Height(), 1,
		                pixelFormat,
		                pixelData,
		                imgs[i].Pixels());
	glPixelStorei(GL_UNPACK_ALIGNMENT,align);
	glPixelStorei(GL_UNPACK_SWAP_BYTES,swapBytes);

	if(genMipmaps == GL_TRUE)
		glGenerateMipmap(GL_TEXTURE_3D);

	delete[] imgs;
}



////////////////////////////////////////////////////////////////////////////////
// Functions implementation
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Next power of two
bool is_power_of_two(GLuint number) {
	return ((number != 0) && !(number & (number - 1)));
}


////////////////////////////////////////////////////////////////////////////////
// Next power of two
GLuint next_power_of_two(GLuint number) {
	if (number == 0)
		return 1;
	--number;
	for (GLuint i=1; i<sizeof(GLuint)*CHAR_BIT; i<<=1)
		number = number | number >> i;
	return number+1;
}


////////////////////////////////////////////////////////////////////////////////
// Next power of two exponent
GLuint next_power_of_two_exponent(GLuint number) {
	if(number==0)
		return 1;

	GLuint pot = next_power_of_two(number);
	GLuint exp = 0;
	while(!(pot & 0x01)){
		pot >>= 1;
		++exp;
	}
	return exp;
}


////////////////////////////////////////////////////////////////////////////////
// build glsl program
GLvoid build_glsl_program(GLuint program,
                          const std::string& srcfile,
                          const std::string& options,
                          GLboolean link ) throw(FWException) {
	// open source file
	std::ifstream file(srcfile.c_str());
	if(file.fail())
		throw _FileNotFoundException(srcfile);

	// check first line (must be the version specification)
	std::string source;
	getline(file, source);
	if(source.find("#version") == std::string::npos)
		throw _ProgramInvalidFirstLineException(srcfile);

	// add the endline character
	source += '\n';

	// add options (if any)
	if(!options.empty())
		source += options + '\n';

	// backup position
	const size_t posbu = source.length();

	// recover whole source
	std::string line;
	while(getline(file, line))
		source += line + '\n';

	try {
		// find different stages and build shaders
		if(source.find("_VERTEX_") != std::string::npos) {
			std::string vsource = source;
			vsource.insert(posbu, "#define _VERTEX_\n");
			_attach_shader(program, GL_VERTEX_SHADER, vsource.data());
		}
		if(source.find("_TESS_CONTROL_") != std::string::npos) {
			std::string csource = source;
			csource.insert(posbu, "#define _TESS_CONTROL_\n");
			_attach_shader(program, GL_TESS_CONTROL_SHADER, csource.data());
		}
		if(source.find("_TESS_EVALUATION_") != std::string::npos) {
			std::string esource = source;
			esource.insert(posbu, "#define _TESS_EVALUATION_\n");
			_attach_shader(program, GL_TESS_EVALUATION_SHADER, esource.data());
		}
		if(source.find("_GEOMETRY_") != std::string::npos) {
			std::string gsource = source;
			gsource.insert(posbu, "#define _GEOMETRY_\n");
			_attach_shader(program, GL_GEOMETRY_SHADER, gsource.data());
		}
		if(source.find("_FRAGMENT_") != std::string::npos) {
			std::string fsource = source;
			fsource.insert(posbu, "#define _FRAGMENT_\n");
			_attach_shader(program, GL_FRAGMENT_SHADER, fsource.data());
		}
	}
	catch(FWException& e) {
		throw _ProgramBuildFailException(srcfile, e.what());
	}

	// Link program if requested
	if(GL_TRUE == link) {
		glLinkProgram(program);
		// check link
		GLint linkStatus = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if(GL_FALSE == linkStatus) {
			GLchar logContent[1024]; // 1kB
			glGetProgramInfoLog(program, 1024, NULL, logContent);
			throw _ProgramLinkFailException(srcfile, logContent);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Check OpenGL error
GLvoid check_gl_error() throw (FWException) {
	GLenum error = glGetError();
	if(GL_NO_ERROR != error) {
		throw _GLErrorException(_gl_error_to_string(error));
	}
}

GLvoid check_framebuffer_status() throw (FWException) {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(GL_FRAMEBUFFER_COMPLETE != status) {
		throw _GLFramebufferStatusException(
			_gl_framebuffer_status_to_string(status));
	}
}


GLvoid init_debug_output(std::ostream& outputStream) throw (FWException) {
	static bool isArbDebugOutputConfigured = false;
	if(!GLEW_ARB_debug_output)  
		throw _DebugOutputNotSupportedException();
	if(isArbDebugOutputConfigured)
	 	throw _DebugOutputAlreadyConfiguredException();
	 	
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(
		reinterpret_cast<GLDEBUGPROCARB>(&_gl_debug_message_callback),
		reinterpret_cast<GLvoid*>(&outputStream) );
	isArbDebugOutputConfigured = true;
	
}

////////////////////////////////////////////////////////////////////////////////
// Take a screenshot
GLvoid save_gl_front_buffer(GLint x,
	                        GLint y,
	                        GLsizei width,
	                        GLsizei height) throw(FWException) {
	_save_gl_buffer(x,y,width,height,GL_FRONT);
}

GLvoid save_gl_back_buffer(GLint x,
	                       GLint y,
	                       GLsizei width,
	                       GLsizei height) throw(FWException) {
	_save_gl_buffer(x,y,width,height,GL_BACK);
}


////////////////////////////////////////////////////////////////////////////////
// Pack to uint_2_10_10_10_rev
GLuint pack_4f_to_uint_2_10_10_10_rev(GLfloat x,
                                      GLfloat y,
                                      GLfloat z,
                                      GLfloat w) {
	// clamp and expand
	GLuint ix = x*1023.0f;
	GLuint iy = y*1023.0f;
	GLuint iz = z*1023.0f;
	GLuint iw = w*3.0f;

	// pack
	GLuint pack;                   // msb                                 lsb
	pack = 0x000001FFu & ix;       // ---- ---- ---- ---- ---- --xx xxxx xxxx
	pack|= 0x000FFC00u & iy << 10; // ---- ---- ---- yyyy yyyy yyxx xxxx xxxx
	pack|= 0x1FF00000u & iz << 20; // --zz zzzz zzzz yyyy yyyy yyxx xxxx xxxx
	pack|= 0xC0000000u & iw << 30; // wwzz zzzz zzzz yyyy yyyy yyxx xxxx xxxx
	return pack;
}

GLuint pack_4fv_to_uint_2_10_10_10_rev(const GLfloat *v) {
	return pack_4f_to_uint_2_10_10_10_rev(v[0], v[1], v[2], v[3]);
}


////////////////////////////////////////////////////////////////////////////////
// Pack to int_2_10_10_10_rev
GLint pack_4f_to_int_2_10_10_10_rev(GLfloat x,
                                    GLfloat y,
                                    GLfloat z,
                                    GLfloat w) {
	// clamp and expand
	GLint ix = x*511.0f;
	GLint iy = y*511.0f;
	GLint iz = z*511.0f;
	GLint iw = w;

	// pack
	GLuint pack;                  // msb                                 lsb
	pack = 0x000001FF & ix;       // ---- ---- ---- ---- ---- --xx xxxx xxxx
	pack|= 0x000FFC00 & iy << 10; // ---- ---- ---- yyyy yyyy yyxx xxxx xxxx
	pack|= 0x1FF00000 & iz << 20; // --zz zzzz zzzz yyyy yyyy yyxx xxxx xxxx
	pack|= 0xC0000000 & iw << 30; // wwzz zzzz zzzz yyyy yyyy yyxx xxxx xxxx
	return pack;
}

GLint pack_4fv_to_int_2_10_10_10_rev(const GLfloat *v) {
	return pack_4f_to_int_2_10_10_10_rev(v[0], v[1], v[2], v[3]);
}


////////////////////////////////////////////////////////////////////////////////
// RGB packing (saves most significant bits)
GLubyte pack_3ub_to_ubyte_3_3_2(GLubyte r,
	                            GLubyte g,
	                            GLubyte b) {
	return (r & 0xE0u) | (g >> 3 & 0x1Cu) | (b >> 6 & 0x03u); // rrrg ggbb
}

GLushort pack_3ub_to_ushort_4_4_4(GLubyte r,
	                              GLubyte g,
	                              GLubyte b) {
	GLushort pack;               // msb             lsb
	pack = (0x0F00u & (r << 4)); // ---- rrrr ---- ----
	pack|= (0x00F0u & g);        // ---- rrrr gggg ----
	pack|= (0x000Fu & (b >> 4)); // ---- rrrr gggg bbbb
	return pack;
}

GLushort pack_3ub_to_ushort_5_5_5(GLubyte r,
	                              GLubyte g,
	                              GLubyte b) {
	GLushort pack;               // msb             lsb
	pack = (0x7C00u & (r << 7)); // -rrr rr-- ---- ----
	pack|= (0x03E0u & (g << 2)); // -rrr rrgg ggg- ----
	pack|= (0x001Fu & (b >> 3)); // -rrr rrgg gggb bbbb
	return pack;
}

GLushort pack_3ub_to_ushort_5_6_5(GLubyte r,
	                              GLubyte g,
	                              GLubyte b) {
	GLushort pack;               // msb             lsb
	pack = (0xF800u & (r << 8)); // rrrr r--- ---- ----
	pack|= (0x07E0u & (g << 3)); // rrrr rggg ggg- ----
	pack|= (0x001Fu & (b >> 3)); // rrrr rggg gggb bbbb
	return pack;
}

GLubyte pack_3ubv_to_ubyte_3_3_2(const GLubyte *v) {
	return pack_3ub_to_ubyte_3_3_2(v[0],v[1],v[2]);
}

GLushort pack_3ubv_to_ushort_4_4_4(const GLubyte *v) {
	return pack_3ub_to_ushort_4_4_4(v[0],v[1],v[2]);
}

GLushort pack_3ubv_to_ushort_5_5_5(const GLubyte *v) {
	return pack_3ub_to_ushort_5_5_5(v[0],v[1],v[2]);
}

GLushort pack_3ubv_to_ushort_5_6_5(const GLubyte *v) {
	return pack_3ub_to_ushort_5_6_5(v[0],v[1],v[2]);
}


////////////////////////////////////////////////////////////////////////////////
// RGBA packing (saves most significant bits)
GLushort pack_4ub_to_ushort_4_4_4_4(GLubyte r,
                                    GLubyte g,
                                    GLubyte b,
                                    GLubyte a) {
	GLushort pack;               // msb             lsb
	pack = (0xF000u & (r << 8)); // rrrr ---- ---- ----
	pack|= (0x0F00u & (g << 4)); // rrrr gggg ---- ----
	pack|= (0x00F0u & b);        // rrrr gggg bbbb ----
	pack|= (0x000Fu & (a >> 4)); // rrrr gggg bbbb aaaa
	return pack;
}

GLushort pack_4ub_to_ushort_5_5_5_1(GLubyte r,
                                    GLubyte g,
                                    GLubyte b,
                                    GLubyte a) {
	GLushort pack;               // msb             lsb
	pack = (0xF800u & (r << 8)); // rrrr r--- ---- ----
	pack|= (0x07C0u & (g << 3)); // rrrr rggg gg-- ----
	pack|= (0x003Eu & (b >> 2)); // rrrr rggg ggbb bbb-
	pack|= (0x0001u & (a >> 7)); // rrrr rggg ggbb bbba
	return pack;
}

GLushort pack_4ubv_to_ushort_4_4_4_4(const GLubyte *v) {
	return pack_4ub_to_ushort_4_4_4_4(v[0],v[1],v[2],v[3]);
}

GLushort pack_4ubv_to_ushort_5_5_5_1(const GLubyte *v) {
	return pack_4ub_to_ushort_5_5_5_1(v[0],v[1],v[2],v[3]);
}


////////////////////////////////////////////////////////////////////////////////
// render with fsaa
#define left   frustum[0]
#define right  frustum[1]
#define bottom frustum[2]
#define top    frustum[3]
#define near   frustum[4]
#define far    frustum[5]

void render_fsaa(GLsizei width,
	             GLsizei height,
	             GLsizei sampleCnt,
	             GLfloat *frustum, // frustum data
	             GLboolean perspective,
	             void (*set_transforms_func)(GLfloat *perspectiveMatrix),
	             void (*draw_func)() ) throw(FWException) {
	const GLfloat frustumScaleX = (right - left) / GLfloat(width);
	const GLfloat frustumScaleY = (top - bottom) / GLfloat(height);
	GLuint framebuffers[3], depthbuffer(0), 
	       aaColourbuffer(0), colourbuffer(0);
	GLint aaMipLevels     = next_power_of_two_exponent(sampleCnt);
	GLint activeRenderbuffer, activeReadFramebuffer, activeDrawFramebuffer,
	      activeReadBuffer, activeDrawBuffer, 
	      activeTextureUnit, activeTexture,
	      activeViewport[4];

	// check inputs
	if(frustum==NULL || 
	   set_transforms_func == NULL || 
	   draw_func == NULL)
		throw _NullParamException();

	// save GL state
	glGetIntegerv(GL_READ_BUFFER, &activeReadBuffer);
	glGetIntegerv(GL_DRAW_BUFFER, &activeDrawBuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
	              &activeDrawFramebuffer);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,
	              &activeReadFramebuffer);
	glGetIntegerv(GL_RENDERBUFFER_BINDING,
	              &activeRenderbuffer);
	glGetIntegerv(GL_ACTIVE_TEXTURE,
	              &activeTextureUnit);
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D,
	              &activeTexture);
	glGetIntegerv(GL_VIEWPORT,
	              activeViewport);

	// create resources
	glGenFramebuffers(3, framebuffers);
	glGenRenderbuffers(1, &depthbuffer);
	glGenTextures(1, &aaColourbuffer);
	glGenTextures(1, &colourbuffer);

	// configure renderbuffers
	glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER,
	                      GL_DEPTH_COMPONENT,
	                      sampleCnt,
	                      sampleCnt);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// configure textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colourbuffer);
	glTexStorage2D(GL_TEXTURE_2D,
	               1,
	               GL_RGBA8,
	               width,
	               height);
	glBindTexture(GL_TEXTURE_2D, aaColourbuffer);
	glTexStorage2D(GL_TEXTURE_2D,
	               aaMipLevels,
	               GL_RGBA8,
	               sampleCnt,
	               sampleCnt);

	// configure framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
	                       GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D,
	                       aaColourbuffer,
	                       aaMipLevels-1);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[2]);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
	                       GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D,
	                       colourbuffer,
	                       0);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,
	                          GL_DEPTH_ATTACHMENT,
	                          GL_RENDERBUFFER,
	                          depthbuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
	                       GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D,
	                       aaColourbuffer,
	                       0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	// reset the texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, activeTexture);
	glActiveTexture(activeTextureUnit);

	// render the scene with sub projections
	glViewport(0,0,sampleCnt,sampleCnt);
	for(GLint x=0; x<width; ++x)
		for(GLint y=0; y<height; ++y) {
			// compute the frustum and projection matrix
			GLfloat scaledMatrix[16];
			GLfloat scaledFrustum[6] = {
				left+x*frustumScaleX,
				left+(x+1)*frustumScaleX,
				bottom+y*frustumScaleY,
				bottom+(y+1)*frustumScaleY,
				near, far };
			_frustum_matrix(scaledFrustum, perspective, scaledMatrix);
			
			// set uniforms
			set_transforms_func(scaledMatrix);

			// draw the scene
			draw_func();

			// mipmap and copy data to framebuffer
			glBindTexture(GL_TEXTURE_2D, aaColourbuffer);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
			glBindTexture(GL_TEXTURE_2D, colourbuffer);
			glCopyTexSubImage2D(GL_TEXTURE_2D,
			                    0,
			                    x,y,
			                    0,0,
			                    1,1);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
		}

	// render to back buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers[2]);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glBlitFramebuffer(0,0,width,height,0,0,width,height,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// restore GL state (order matters !)
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, activeDrawFramebuffer);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, activeReadFramebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, activeRenderbuffer);
	glReadBuffer(activeReadBuffer);
	glDrawBuffer(activeDrawBuffer);
	glViewport(activeViewport[0],activeViewport[1],
	           activeViewport[2],activeViewport[3]);

	// delete resources
	glDeleteFramebuffers(3, framebuffers);
	glDeleteRenderbuffers(1, &depthbuffer);
	glDeleteTextures(1, &aaColourbuffer);
	glDeleteTextures(1, &colourbuffer);
}

#undef left
#undef right
#undef bottom
#undef top
#undef near
#undef far


////////////////////////////////////////////////////////////////////////////////
// Upload a TGA image to a texture
static void _extract_tga_format(const Tga& tga,
                                GLenum &internalFormat,
                                GLenum &pixelFormat) {
	internalFormat = GL_R8;
	pixelFormat = GL_RED;
	if(tga.PixelFormat() == Tga::PIXEL_FORMAT_LUMINANCE_ALPHA) {
		internalFormat = GL_RG8;
		pixelFormat = GL_RG;
	}
	else if(tga.PixelFormat() == Tga::PIXEL_FORMAT_BGR) {
		internalFormat = GL_RGB8;
		pixelFormat = GL_BGR;
	}
	else if(tga.PixelFormat() == Tga::PIXEL_FORMAT_BGRA) {
		internalFormat = GL_RGBA8;
		pixelFormat = GL_BGRA;
	}
}


void tex_tga_image2D(const std::string& filename,
                     GLboolean genMipmaps,
                     GLboolean immutable) throw(FWException) {
	tex_img_image2D<Tga>(filename,
	                     genMipmaps,
	                     immutable,
	                     &_extract_tga_format);
}

void tex_tga_cube_map(const std::string filenames[6],
                      GLboolean genMipmaps,
                      GLboolean immutable) throw(FWException) {
	tex_img_cube_map<Tga>(filenames,
	                      genMipmaps,
	                      immutable,
	                      &_extract_tga_format);
}

void tex_tga_sprites_image3D(const std::vector<std::string>& filenames,
	                         GLboolean genMipmaps,
	                         GLboolean immutable) throw(FWException) {
	tex_img_sprites_image3D<Tga>(filenames,
	                             genMipmaps,
	                             immutable,
	                             &_extract_tga_format);
}


#ifndef _NO_PNG
////////////////////////////////////////////////////////////////////////////////
// Upload a PNG image to a texture
static void _extract_png_format(const Png& png,
                                GLenum &internalFormat,
                                GLenum &pixelFormat) {
	if(png.PixelFormat()==Png::PIXEL_FORMAT_LUMINANCE) {
		pixelFormat = GL_RED;
		internalFormat = png.BitsPerPixel()==8 ? GL_R8 : GL_R16;
	}
	else if(png.PixelFormat()==Png::PIXEL_FORMAT_LUMINANCE_ALPHA) {
		pixelFormat = GL_RG;
		internalFormat = png.BitsPerPixel()==8 ? GL_RG8 : GL_RG16;
	}
	else if(png.PixelFormat()==Png::PIXEL_FORMAT_RGB) {
		pixelFormat = GL_RGB;
		internalFormat = png.BitsPerPixel()==8 ? GL_RGB8 : GL_RGB16;
	}
	else if(png.PixelFormat()==Png::PIXEL_FORMAT_RGBA) {
		pixelFormat = GL_RGBA;
		internalFormat = png.BitsPerPixel()==8 ? GL_RGBA8 : GL_RGBA16;
	}
}

void tex_png_image2D(const std::string& filename,
                     GLboolean genMipmaps,
                     GLboolean immutable) throw(FWException) {
	tex_img_image2D<Png>(filename,
	                     genMipmaps,
	                     immutable,
	                     &_extract_png_format);
}

void tex_png_cube_map(const std::string filenames[6],
                      GLboolean genMipmaps,
                      GLboolean immutable) throw(FWException) {
	tex_img_cube_map<Png>(filenames,
	                      genMipmaps,
	                      immutable,
	                      _extract_png_format);
}


void tex_png_sprites_image3D(const std::vector<std::string>& filenames,
	                         GLboolean genMipmaps,
	                         GLboolean immutable) throw(FWException) {
	tex_img_sprites_image3D<Png>(filenames,
	                             genMipmaps,
	                             immutable,
	                             &_extract_png_format);
}

#endif // no png

////////////////////////////////////////////////////////////////////////////////
// Half to float and float to half conversions
// Left the work to others (see comments below)

/*
 * Ork: a small object-oriented OpenGL Rendering Kernel.
 * Copyright (c) 2008-2010 INRIA
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 */

/*
 * Authors: Eric Bruneton, Antoine Begault, Guillaume Piolat.
 */

// Code derived from:
// http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvmath/Half.cpp

// Branch-free implementation of half-precision (16 bit) floating point
// Copyright 2006 Mike Acton <macton@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE
//
// Half-precision floating point format
// ------------------------------------
//
//   | Field    | Last | First | Note
//   |----------|------|-------|----------
//   | Sign     | 15   | 15    |
//   | Exponent | 14   | 10    | Bias = 15
//   | Mantissa | 9    | 0     |
//
// Compiling
// ---------
//
//  Preferred compile flags for GCC:
//     -O3 -fstrict-aliasing -std=c99 -pedantic -Wall -Wstrict-aliasing
//
//     This file is a C99 source file, intended to be compiled with a C99
//     compliant compiler. However, for the moment it remains combatible
//     with C++98. Therefore if you are using a compiler that poorly implements
//     C standards (e.g. MSVC), it may be compiled as C++. This is not
//     guaranteed for future versions.
//
// Features
// --------
//
//  * QNaN + <x>  = QNaN
//  * <x>  + +INF = +INF
//  * <x>  - -INF = -INF
//  * INF  - INF  = SNaN
//  * Denormalized values
//  * Difference of ZEROs is always +ZERO
//  * Sum round with guard + round + sticky bit (grs)
//  * And of course... no branching
//
// Precision of Sum
// ----------------
//
//  (SUM)        uint16 z = half_add( x, y );
//  (DIFFERENCE) uint16 z = half_add( x, -y );
//
//     Will have exactly (0 ulps difference) the same result as:
//     (For 32 bit IEEE 784 floating point and same rounding mode)
//
//     union FLOAT_32
//     {
//       float    f32;
//       uint32 u32;
//     };
//
//     union FLOAT_32 fx = { .u32 = half_to_float( x ) };
//     union FLOAT_32 fy = { .u32 = half_to_float( y ) };
//     union FLOAT_32 fz = { .f32 = fx.f32 + fy.f32    };
//     uint16       z  = float_to_half( fz );
//

// Negate
#ifdef _MSC_VER
// prevent a MSVC warning
#   define _unsigned_neg(a) (~(a) + 1)
#else
#   define _unsigned_neg(a) (-(a))
#endif

// Select on Sign bit
static inline unsigned _unsigned_sels(unsigned test, unsigned a, unsigned b)
{
    unsigned mask = ((int)test) >> 31; // sign-extend
    unsigned sel_a  = a & mask;
    unsigned sel_b  = b & (~mask);
    unsigned result =  sel_a | sel_b;
    return result;
}

// Count Leading Zeros
static inline unsigned _unsigned_cntlz(unsigned x)
{
#ifdef __GNUC__
  /* On PowerPC, this will map to insn: cntlzw */
  /* On Pentium, this will map to insn: clz    */
    unsigned nlz = __builtin_clz(x);
    return nlz;
#else
    unsigned x0  = x >> 1l;
    unsigned x1  = x | x0;
    unsigned x2  = x1 >> 2l;
    unsigned x3  = x1 | x2;
    unsigned x4  = x3 >> 4l;
    unsigned x5  = x3 | x4;
    unsigned x6  = x5 >> 8l;
    unsigned x7  = x5 | x6;
    unsigned x8  = x7 >> 16l;
    unsigned x9  = x7 | x8;
    unsigned xA  = ~x9;
    unsigned xB  = xA >> 1l;
    unsigned xC  = xB & 0x55555555;
    unsigned xD  = xA - xC;
    unsigned xE  = xD & 0x33333333;
    unsigned xF  = xD >> 2l;
    unsigned x10 = xF & 0x33333333;
    unsigned x11 = xE + x10;
    unsigned x12 = x11 >> 4l;
    unsigned x13 = x11 + x12;
    unsigned x14 = x13 & 0x0f0f0f0f;
    unsigned x15 = x14 >> 8l;
    unsigned x16 = x14 + x15;
    unsigned x17 = x16 >> 16l;
    unsigned x18 = x16 + x17;
    unsigned x19 = x18 & 0x0000003f;
    return x19;
#endif
}

GLhalf float_to_half(float x)
{
	union
	{
		float f32;
		unsigned ui32;
	} u;

	u.f32 = x;
	unsigned f = u.ui32;

	unsigned one                        = 0x00000001;
	unsigned f_e_mask                   = 0x7f800000;
	unsigned f_m_mask                   = 0x007fffff;
	unsigned f_s_mask                   = 0x80000000;
	unsigned h_e_mask                   = 0x00007c00;
	unsigned f_e_pos                    = 0x00000017;
	unsigned f_m_round_bit              = 0x00001000;
	unsigned h_nan_em_min               = 0x00007c01;
	unsigned f_h_s_pos_offset           = 0x00000010;
	unsigned f_m_hidden_bit             = 0x00800000;
	unsigned f_h_m_pos_offset           = 0x0000000d;
	unsigned f_h_bias_offset            = 0x38000000;
	unsigned f_m_snan_mask              = 0x003fffff;
	unsigned h_snan_mask                = 0x00007e00;
	unsigned f_e                        = f & f_e_mask;
	unsigned f_m                        = f & f_m_mask;
	unsigned f_s                        = f & f_s_mask;
	unsigned f_e_h_bias                 = f_e - f_h_bias_offset;
	unsigned f_e_h_bias_amount          = f_e_h_bias >> (int)f_e_pos;
	unsigned f_m_round_mask             = f_m & f_m_round_bit;
	unsigned f_m_round_offset           = f_m_round_mask << 1;
	unsigned f_m_rounded                = f_m + f_m_round_offset;
	unsigned f_m_rounded_overflow       = f_m_rounded & f_m_hidden_bit;
	unsigned f_m_denorm_sa              = one - f_e_h_bias_amount;
	unsigned f_m_with_hidden            = f_m_rounded | f_m_hidden_bit;
	unsigned f_m_denorm                 = f_m_with_hidden >> (int)f_m_denorm_sa;
	unsigned f_em_norm_packed           = f_e_h_bias | f_m_rounded;
	unsigned f_e_overflow               = f_e_h_bias + f_m_hidden_bit;
	unsigned h_s                        = f_s >> (int)f_h_s_pos_offset;
	unsigned h_m_nan                    = f_m >> (int)f_h_m_pos_offset;
	unsigned h_m_denorm                 = f_m_denorm >> (int)f_h_m_pos_offset;
	unsigned h_em_norm                  = f_em_norm_packed >> (int)f_h_m_pos_offset;
	unsigned h_em_overflow              = f_e_overflow >> (int)f_h_m_pos_offset;
	unsigned is_e_eqz_msb               = f_e - 1;
	unsigned is_m_nez_msb               = _unsigned_neg(f_m);
	unsigned is_h_m_nan_nez_msb         = _unsigned_neg(h_m_nan);
	unsigned is_e_nflagged_msb          = f_e - f_e_mask;
	unsigned is_ninf_msb                = is_e_nflagged_msb | is_m_nez_msb;
	unsigned is_underflow_msb           = is_e_eqz_msb - f_h_bias_offset;
	unsigned is_nan_nunderflow_msb      = is_h_m_nan_nez_msb | is_e_nflagged_msb;
	unsigned is_m_snan_msb              = f_m_snan_mask - f_m;
	unsigned is_snan_msb                = is_m_snan_msb & (~is_e_nflagged_msb);
	unsigned is_overflow_msb            = _unsigned_neg(f_m_rounded_overflow);
	unsigned h_nan_underflow_result     = _unsigned_sels(is_nan_nunderflow_msb, h_em_norm, h_nan_em_min);
	unsigned h_inf_result               = _unsigned_sels(is_ninf_msb, h_nan_underflow_result, h_e_mask);
	unsigned h_underflow_result         = _unsigned_sels(is_underflow_msb, h_m_denorm, h_inf_result);
	unsigned h_overflow_result          = _unsigned_sels(is_overflow_msb, h_em_overflow, h_underflow_result);
	unsigned h_em_result                = _unsigned_sels(is_snan_msb, h_snan_mask, h_overflow_result);
	unsigned h_result                   = h_em_result | h_s;
	return h_result;
}


float half_to_float(GLhalf h)
{
	unsigned h_e_mask              = 0x00007c00;
	unsigned h_m_mask              = 0x000003ff;
	unsigned h_s_mask              = 0x00008000;
	unsigned h_f_s_pos_offset      = 0x00000010;
	unsigned h_f_e_pos_offset      = 0x0000000d;
	unsigned h_f_bias_offset       = 0x0001c000;
	unsigned f_e_mask              = 0x7f800000;
	unsigned f_m_mask              = 0x007fffff;
	unsigned h_f_e_denorm_bias     = 0x0000007e;
	unsigned h_f_m_denorm_sa_bias  = 0x00000008;
	unsigned f_e_pos               = 0x00000017;
	unsigned h_e_mask_minus_one    = 0x00007bff;
	unsigned h_e                   = h & h_e_mask;
	unsigned h_m                   = h & h_m_mask;
	unsigned h_s                   = h & h_s_mask;
	unsigned h_e_f_bias            = h_e + h_f_bias_offset;
	unsigned h_m_nlz               = _unsigned_cntlz(h_m);
	unsigned f_s                   = h_s << h_f_s_pos_offset;
	unsigned f_e                   = h_e_f_bias << h_f_e_pos_offset;
	unsigned f_m                   = h_m << h_f_e_pos_offset;
	unsigned f_em                  = f_e | f_m;
	unsigned h_f_m_sa              = h_m_nlz - h_f_m_denorm_sa_bias;
	unsigned f_e_denorm_unpacked   = h_f_e_denorm_bias - h_f_m_sa;
	unsigned h_f_m                 = h_m << h_f_m_sa;
	unsigned f_m_denorm            = h_f_m & f_m_mask;
	unsigned f_e_denorm            = f_e_denorm_unpacked << f_e_pos;
	unsigned f_em_denorm           = f_e_denorm | f_m_denorm;
	unsigned f_em_nan              = f_e_mask | f_m;
	unsigned is_e_eqz_msb          = h_e - 1;
	unsigned is_m_nez_msb          = _unsigned_neg(h_m);
	unsigned is_e_flagged_msb      = h_e_mask_minus_one - h_e;
	unsigned is_zero_msb           = is_e_eqz_msb & (~is_m_nez_msb);
	unsigned is_inf_msb            = is_e_flagged_msb & (~is_m_nez_msb);
	unsigned is_denorm_msb         = is_m_nez_msb & is_e_eqz_msb;
	unsigned is_nan_msb            = is_e_flagged_msb & is_m_nez_msb;
	unsigned is_zero               = ((int)is_zero_msb) >> 31; // sign-extend
	unsigned f_zero_result         = f_em & (~is_zero);
	unsigned f_denorm_result       = _unsigned_sels(is_denorm_msb, f_em_denorm, f_zero_result);
	unsigned f_inf_result          = _unsigned_sels(is_inf_msb, f_e_mask, f_denorm_result);
	unsigned f_nan_result          = _unsigned_sels(is_nan_msb, f_em_nan, f_inf_result);
	unsigned f_result              = f_s | f_nan_result;

	union
	{
		float f32;
		unsigned ui32;
	} u;

	u.ui32 = f_result;
	return u.f32;
}


////////////////////////////////////////////////////////////////////////////////
// Timer implementation
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Timer Constructor
Timer::Timer() : 
	mStartTicks(0.0), mStopTicks(0.0), mIsTicking(false)
{}


////////////////////////////////////////////////////////////////////////////////
// Timer::Start 
void Timer::Start()
{
	if(!mIsTicking) {
		mIsTicking  = true;
		mStartTicks = _get_ticks();
	}
}


////////////////////////////////////////////////////////////////////////////////
// Timer::Stop()
void Timer::Stop()
{
	if(mIsTicking) {
		mIsTicking = false;
		mStopTicks = _get_ticks();
	}
}


////////////////////////////////////////////////////////////////////////////////
// Timer::Ticks()
double Timer::Ticks() const
{
	return mIsTicking ? _get_ticks() - mStartTicks 
	                  : mStopTicks - mStartTicks;
}


////////////////////////////////////////////////////////////////////////////////
// Tga local functions/constants
//
////////////////////////////////////////////////////////////////////////////////

// thanks to : http://paulbourke.net/dataformats/tga/
enum _TgaImageType
{
	_TGA_TYPE_CM            = 1,
	_TGA_TYPE_RGB           = 2,
	_TGA_TYPE_LUMINANCE     = 3,
	_TGA_TYPE_CM_RLE        = 9,
	_TGA_TYPE_RGB_RLE       = 10,
	_TGA_TYPE_LUMINANCE_RLE = 11
};


////////////////////////////////////////////////////////////////////////////////
// Tga specific exceptions
//
////////////////////////////////////////////////////////////////////////////////
class _TgaLoaderException : public FWException
{
public:
	_TgaLoaderException(const std::string& filename, const std::string& log)
	{
		mMessage = "In file "+filename+": "+log;
	}
};

class _TgaInvalidDescriptorException : public FWException
{
public:
	_TgaInvalidDescriptorException()
	{
		mMessage = "Invalid TGA image descriptor.";
	}
};

class _TgaInvalidBppValueException : public FWException
{
public:
	_TgaInvalidBppValueException()
	{
		mMessage = "Invalid TGA bits per pixel amount.";
	}
};

class _TgaInvalidCmSizeException : public FWException
{
public:
	_TgaInvalidCmSizeException()
	{
		mMessage = "Invalid TGA colour map size.";
	}
};

class _TgaInvalidImageDescriptorByteException : public FWException
{
public:
	_TgaInvalidImageDescriptorByteException()
	{
		mMessage = "Invalid TGA image descriptor byte.";
	}
};


////////////////////////////////////////////////////////////////////////////////
// Tga implementation
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Extract uint16 from two uint8
GLushort Tga::_UnpackUint16(GLubyte msb, GLubyte lsb) {
	return (static_cast<GLushort>(lsb) | static_cast<GLushort>(msb) << 8);
}


////////////////////////////////////////////////////////////////////////////////
// Flip horizontally
void Tga::_Flip()
{
	GLubyte* flippedData = new GLubyte[mWidth*mHeight*mPixelFormat];
	for(GLushort y = 0; y<mHeight; ++y)
		for(GLushort x = 0; x<mWidth; ++x)
			memcpy( &flippedData[y*mWidth*mPixelFormat+x*mPixelFormat],
			       &mPixels[(mWidth-y)*mWidth*mPixelFormat+x*mPixelFormat],
			       mPixelFormat );
	delete[] mPixels;
	mPixels = flippedData;
}


////////////////////////////////////////////////////////////////////////////////
// colour mapped images
void Tga::_LoadColourMapped( std::ifstream& fileStream,
                             GLchar* header )  throw(FWException)
{
	// set offset
	GLushort offset = _UnpackUint16(header[4], header[3])+header[0];
	fileStream.seekg(offset, std::ifstream::cur);

	// check image descriptor byte
	if(header[17]!=0)
		throw _TgaInvalidDescriptorException();

	// get colourMap indexes
	GLuint indexSize = header[16];
	if(indexSize<1)
		throw _TgaInvalidBppValueException();

	// check cm size
	GLuint colourMapSize = _UnpackUint16(header[5], header[6]);
	if(colourMapSize < 1)
		throw _TgaInvalidCmSizeException();

	GLubyte* colourMap = NULL;
	mPixelFormat = header[7]>>3;
	if(mPixelFormat==4 || mPixelFormat==3)
	{
		GLuint colourMapByteCnt = colourMapSize * mPixelFormat;
		colourMap = new GLubyte[colourMapByteCnt];
		fileStream.read( reinterpret_cast<GLchar*>(colourMap),
		                 colourMapByteCnt );
	}
	else if(mPixelFormat==2)
	{
		colourMapSize = _UnpackUint16(header[5], header[6]);
		GLuint colourMapByteCnt = colourMapSize * 3;
		colourMap = new GLubyte[colourMapByteCnt];

		GLuint maxIter_cm 	= colourMapSize*2;
		GLushort rgb16 			= 0;
		for(GLuint i=0; i<maxIter_cm; ++i)
		{
			fileStream.read(reinterpret_cast<GLchar*>(&rgb16), 2);
			colourMap[i*3]   = static_cast<GLubyte>((rgb16 & 0x001F)<<3);
			colourMap[i*3+1] = static_cast<GLubyte>(((rgb16 & 0x03E0)>>5)<<3);
			colourMap[i*3+2] = static_cast<GLubyte>(((rgb16 & 0x7C00)>>10)<<3);
		}
		mPixelFormat = Tga::PIXEL_FORMAT_BGR; // convert to bgr
	}
	else
		throw _TgaInvalidBppValueException();

	// allocate pixel data
	mPixels = new GLubyte[mWidth*mHeight*mPixelFormat];

	// read data
	GLuint maxIter = static_cast<GLuint>(mWidth)*static_cast<GLuint>(mHeight);
	GLubyte bytesPerIndex = header[16]>>3;
	GLuint index = 0;
	for(GLuint i=0; i<maxIter; ++i)
	{
		// read index
		fileStream.read(reinterpret_cast<GLchar*>(&index), bytesPerIndex);
		// get color
		memcpy( &mPixels[i*mPixelFormat],
		        &colourMap[index*mPixelFormat],
		        mPixelFormat );
	}

	// flip if necessary
	if(1==(header[17]>>5 & 0x01))
		_Flip();

	// free memory
	delete[] colourMap;
}


////////////////////////////////////////////////////////////////////////////////
void Tga::_LoadLuminance( std::ifstream& fileStream,
                          GLchar* header ) throw(FWException)
{
	// set offset
	GLuint offset = header[0]
	              + header[1]
	              * ( _UnpackUint16(header[4], header[3])
	              + _UnpackUint16(header[6], header[5] )
	              * (header[7]>>3));
	fileStream.seekg(offset, std::ifstream::cur);

	// read data depending on bits per pixel
	if(header[16]==8 || header[16]==16)
	{
		mPixelFormat = header[16] >> 3;
		mPixels      = new GLubyte[mWidth*mHeight*mPixelFormat];
		fileStream.read( reinterpret_cast<GLchar*>(mPixels),
		                 mWidth*mHeight*mPixelFormat);
	}
	else
		throw _TgaInvalidBppValueException();

	// flip if necessary
	if(1==(header[17]>>5 & 0x01))
		_Flip();
}


////////////////////////////////////////////////////////////////////////////////
// unmapped
void Tga::_LoadUnmapped( std::ifstream& fileStream,
                         GLchar* header ) throw(FWException)
{
	// set offset
	GLuint offset = header[0]
	              + header[1]
	              * ( _UnpackUint16(header[4], header[3])
	              + _UnpackUint16(header[6], header[5] )
	              * (header[7]>>3));
	fileStream.seekg(offset, std::ifstream::cur);

	// read data depending on bits per pixel
	if(header[16]==16)
	{
		mPixelFormat = 3;
		mPixels = new GLubyte[mWidth*mHeight*mPixelFormat];
		// load variables
		GLuint maxIter  = static_cast<GLuint>(mWidth)
		                * static_cast<GLuint>(mHeight);
		GLushort rgb16  = 0;
		for(GLuint i=0; i<maxIter; ++i)
		{
			fileStream.read(reinterpret_cast<GLchar*>(&rgb16), 2);
			mPixels[i*3]     = static_cast<GLubyte>((rgb16 & 0x001F)<<3);
			mPixels[i*3+1]   = static_cast<GLubyte>(((rgb16 & 0x03E0)>>5)<<3);
			mPixels[i*3+2]   = static_cast<GLubyte>(((rgb16 & 0x7C00)>>10)<<3);
		}
	}
	else if(header[16]==24 || header[16]==32)
	{
		mPixelFormat = header[16] >> 3;
		mPixels      = new GLubyte[mWidth*mHeight*mPixelFormat];
		fileStream.read( reinterpret_cast<GLchar*>(mPixels),
		                 mWidth*mHeight*mPixelFormat );
	}
	else
		throw _TgaInvalidBppValueException();

	// flip if necessary
	if(1==(header[17]>>5 & 0x01))
		_Flip();
}


////////////////////////////////////////////////////////////////////////////////
// unmapped rle
void Tga::_LoadUnmappedRle( std::ifstream& fileStream,
                            GLchar* header ) throw(FWException)
{
	// set offset
	GLuint offset = header[0]
	              + header[1]
	              * ( _UnpackUint16(header[4], header[3])
	              + _UnpackUint16(header[6], header[5] )
	              * (header[7]>>3));
	fileStream.seekg(offset, std::ifstream::cur);

	// read data depending on bits per pixel
	if(header[16]==16)
	{
		mPixelFormat = 3;   // will be converted to bgr
		mPixels = new GLubyte[mWidth*mHeight*mPixelFormat];
		GLubyte* dataPtrMax = mPixels + (mWidth*mHeight*mPixelFormat);
		GLubyte* dataPtr = mPixels;
		GLint blockSize, blockIter;
		GLubyte packetHeader = 0;
		GLushort rgb16 = 0;  // compressed data
		while(dataPtr < dataPtrMax)
		{
			// reset data block iterator
			blockIter = 1;
			// read first byte
			fileStream.read( reinterpret_cast<GLchar*>(&packetHeader),
			                 sizeof(GLubyte));
			blockSize = 1 + (packetHeader & 0x7F);
			fileStream.read(reinterpret_cast<GLchar*>(&rgb16), 16);  // read 16 bits
			dataPtr[0] = static_cast<GLubyte>((rgb16 & 0x001F) << 3);
			dataPtr[1] = static_cast<GLubyte>(((rgb16 & 0x03E0) >> 5) << 3);
			dataPtr[2] = static_cast<GLubyte>(((rgb16 & 0x7C00) >> 10) << 3);
			if(packetHeader & 0x80)   // rle
			{
				for(; blockIter<blockSize; ++blockIter)
					memcpy( dataPtr + blockIter * mPixelFormat,
					        dataPtr,
					        mPixelFormat );
			}
			else
				for(; blockIter<blockSize; ++blockIter)
				{
					fileStream.read(reinterpret_cast<GLchar*>(&rgb16), 16);  // read 16 bits
					dataPtr[blockIter*mPixelFormat+0] = static_cast<GLubyte>((rgb16 & 0x001F) << 3);
					dataPtr[blockIter*mPixelFormat+1] = static_cast<GLubyte>(((rgb16 & 0x03E0) >> 5) << 3);
					dataPtr[blockIter*mPixelFormat+2] = static_cast<GLubyte>(((rgb16 & 0x7C00) >> 10) << 3);
				}
			// increment pixel ptr
			dataPtr+=mPixelFormat*blockSize;
		}
	}
	else if(header[16]==24 || header[16]==32)
	{
		mPixelFormat = header[16] >> 3;
		mPixels      = new GLubyte[mWidth*mHeight*mPixelFormat];

		// read rle
		GLuint blockSize, blockIter;
		GLubyte* dataPtrMax  = mPixels + mWidth*mHeight*mPixelFormat;
		GLubyte* dataPtr     = mPixels;
		GLubyte packetHeader = 0u;
		while(dataPtr < dataPtrMax)
		{
			// reset data block iterator
			blockIter = 1u;
			// read first byte
			fileStream.read(reinterpret_cast<GLchar*>(&packetHeader), 1);
			blockSize = 1u + (packetHeader & 0x7F);
			fileStream.read(reinterpret_cast<GLchar*>(dataPtr), mPixelFormat);
			if(packetHeader & 0x80u)   // rle
				for(; blockIter<blockSize; ++blockIter)
					memcpy(dataPtr+blockIter*mPixelFormat, dataPtr, mPixelFormat);
			else
				for(; blockIter<blockSize; ++blockIter)
					fileStream.read(reinterpret_cast<GLchar*>(dataPtr
					                                         +blockIter
					                                         *mPixelFormat),
					                mPixelFormat);
			// increment pixel ptr
			dataPtr+=mPixelFormat*blockSize;
		}
	}
	else
		throw _TgaInvalidBppValueException();


	// flip if necessary
	if(1==(header[17]>>5 & 0x01))
		_Flip();
}


////////////////////////////////////////////////////////////////////////////////
// colour mapped images rle
void Tga::_LoadColourMappedRle( std::ifstream& fileStream,
                                GLchar* header) throw(FWException)
{
	// set offset
	GLushort offset = _UnpackUint16(header[4], header[3])+header[0];
	fileStream.seekg(offset, std::ifstream::cur);

	// check image descriptor byte
	if(header[17]!=0)
		throw _TgaInvalidImageDescriptorByteException();

	// get colourMap indexes
	GLuint indexSize = header[16];
	if(indexSize<1)
		throw _TgaInvalidBppValueException();

	// check cm size
	GLuint colourMapSize = _UnpackUint16(header[5], header[6]);
	if(colourMapSize < 1)
		throw _TgaInvalidCmSizeException();

	GLubyte* colourMap = NULL;
	mPixelFormat = header[7]>>3;
	if(mPixelFormat==4 || mPixelFormat==3)
	{
		GLuint colourMapByteCnt = colourMapSize * mPixelFormat;
		colourMap = new GLubyte[colourMapByteCnt];
		fileStream.read(reinterpret_cast<GLchar*>(colourMap), colourMapByteCnt);
	}
	else if(mPixelFormat==2)
	{
		colourMapSize = _UnpackUint16(header[5], header[6]);
		GLuint colourMapByteCnt = colourMapSize * 3;
		colourMap = new GLubyte[colourMapByteCnt];

		GLuint maxIter_cm = colourMapSize*2;
		GLushort rgb16    = 0;
		for(GLuint i=0; i<maxIter_cm; ++i)
		{
			fileStream.read(reinterpret_cast<GLchar*>(&rgb16), 2);
			colourMap[i*3]   = static_cast<GLubyte>((rgb16 & 0x001F)<<3);
			colourMap[i*3+1] = static_cast<GLubyte>(((rgb16 & 0x03E0)>>5)<<3);
			colourMap[i*3+2] = static_cast<GLubyte>(((rgb16 & 0x7C00)>>10)<<3);
		}
		mPixelFormat = 3; // convert to bgr
	}
	else
		throw _TgaInvalidBppValueException();

	// allocate pixel data
	mPixels = new GLubyte[mWidth*mHeight*mPixelFormat];

	// read rle
	GLubyte* dataPtrMax = mPixels + mWidth*mHeight*mPixelFormat;
	GLubyte* dataPtr = mPixels;
	GLuint blockSize, blockIter;
	GLubyte packetHeader = 0u;
	GLubyte bytesPerIndex = header[16]>>3;
	GLuint index = 0;
	while(dataPtr < dataPtrMax)
	{
		// reset data block iterator
		blockIter = 1u;
		// read first byte
		fileStream.read(reinterpret_cast<GLchar*>(&packetHeader), 1);
		blockSize = 1u + (packetHeader & 0x7F);
		fileStream.read(reinterpret_cast<GLchar*>(&index), bytesPerIndex);
		memcpy(dataPtr, &colourMap[index*mPixelFormat], mPixelFormat);
		if(packetHeader & 0x80u)   // rle
			for(; blockIter<blockSize; ++blockIter)
				memcpy(dataPtr+blockIter*mPixelFormat, dataPtr, mPixelFormat);
		else // raw
			for(; blockIter<blockSize; ++blockIter)
			{
				fileStream.read(reinterpret_cast<GLchar*>(&index),
				                bytesPerIndex);
				memcpy( dataPtr+blockIter*mPixelFormat,
				        &colourMap[index*mPixelFormat], 
				        mPixelFormat );
			}
		// increment pixel ptr
		dataPtr+=blockSize*mPixelFormat;
	}

	// flip if necessary
	if(1==(header[17]>>5 & 0x01))
		_Flip();

	// free memory
	delete[] colourMap;
}

////////////////////////////////////////////////////////////////////////////////
// luminance images rle
void Tga::_LoadLuminanceRle( std::ifstream& fileStream,
                             GLchar* header) throw(FWException) {
	// set offset
	GLuint offset = header[0]
	              + header[1]
	              *( _UnpackUint16(header[4], header[3])
	              + _UnpackUint16(header[6], header[5])
	              * (header[7]>>3) );
	fileStream.seekg(offset, std::ifstream::cur);

	// read data depending on bits per pixel
	if(header[16]==8 || header[16]==16)
	{
		mPixelFormat = header[16] >> 3;
		mPixels      = new GLubyte[mWidth*mHeight*mPixelFormat];

		// read rle
		GLubyte* dataPtrMax = mPixels + mWidth*mHeight*mPixelFormat;
		GLubyte* dataPtr = mPixels;
		GLuint blockSize, blockIter;
		GLubyte packetHeader = 0u;
		while(dataPtr < dataPtrMax)
		{
			// reset data block iterator
			blockIter = 1u;
			// read first byte
			fileStream.read(reinterpret_cast<GLchar*>(&packetHeader), 1);
			blockSize = 1u + (packetHeader & 0x7F);
			fileStream.read(reinterpret_cast<GLchar*>(dataPtr), mPixelFormat);
			if(packetHeader & 0x80u)   // rle
				for(; blockIter<blockSize; ++blockIter)
					memcpy( dataPtr+blockIter*mPixelFormat,
					        dataPtr,
					        mPixelFormat );
			else
				for(; blockIter<blockSize; ++blockIter)
					fileStream.read( reinterpret_cast<GLchar*>(dataPtr
					                                          +blockIter
					                                          *mPixelFormat),
					                 mPixelFormat );
			// increment pixel ptr
			dataPtr+=mPixelFormat*blockSize;
		}
	}
	else
		throw _TgaInvalidBppValueException();

	// flip if necessary
	if(1==(header[17]>>5 & 0x01))
		_Flip();
}


////////////////////////////////////////////////////////////////////////////////
// Default constructor
Tga::Tga(): 
mPixels(NULL),
mPixelFormat(PIXEL_FORMAT_UNKNOWN),
mWidth(0), mHeight(0) {
}


////////////////////////////////////////////////////////////////////////////////
// Overloaded constructor
Tga::Tga(const std::string& filename) throw(FWException):
mPixels(NULL),
mPixelFormat(PIXEL_FORMAT_UNKNOWN),
mWidth(0), mHeight(0) {
	Load(filename);
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
Tga::~Tga() {
	_Clear();
}


////////////////////////////////////////////////////////////////////////////////
// Load from file
void Tga::Load(const std::string& filename) throw(FWException) {
	// Clear memory if necessary
	_Clear();

	std::ifstream fileStream( filename.c_str(),
	                          std::ifstream::in | std::ifstream::binary );
	if(!fileStream)
		throw _FileNotFoundException(filename);

	// read header
	GLchar header[18];   // header is 18 bytes
	fileStream.read(reinterpret_cast<char*>(header), 18);

	// get data
	mWidth  = _UnpackUint16(header[13],header[12]);
	mHeight = _UnpackUint16(header[15],header[14]);

	// check image dimensions and pixel data
	if(0 == mWidth * mHeight)
		throw _TgaLoaderException(filename, "Invalid TGA dimensions.");

	// load data according to image type code
	try {
		if(header[2]==_TGA_TYPE_RGB)
			_LoadUnmapped(fileStream, header);
		else if(header[2]==_TGA_TYPE_CM)
			_LoadColourMapped(fileStream, header);
		else if(header[2]==_TGA_TYPE_LUMINANCE)
			_LoadLuminance(fileStream, header);
		else if(header[2]==_TGA_TYPE_CM_RLE)
			_LoadColourMappedRle(fileStream, header);
		else if(header[2]==_TGA_TYPE_RGB_RLE)
			_LoadUnmappedRle(fileStream, header);
		else if(header[2]==_TGA_TYPE_LUMINANCE_RLE)
			_LoadLuminanceRle(fileStream, header);
		else
			throw _TgaLoaderException(filename, "Unknown TGA image type code.");
	}
	catch(FWException& e) {
		throw _TgaLoaderException(filename, e.what());
	}
	catch(...)
	{
		throw _TgaLoaderException(filename, "Unknown error occured.");
	}

	fileStream.close();
}


#if 0
////////////////////////////////////////////////////////////////////////////////
// Save to file
void Tga::Save(const std::string& filename) throw(FWException)
{
	std::ofstream fileStream( filename.c_str(),
	                          std::ifstream::out | std::ifstream::binary );

	if(!fileStream)
		throw _TgaLoaderException(filename, "Could not open file.");

	if(NULL == mPixels)
		throw _TgaLoaderException(filename, "No pixel data in object.");

	// build header
	const GLchar header[18]=
	{
		0,                                 // image identification field
		0,                                 // colormap type
		mPixelFormat < 3 ? 3 : 2,          // image type code
		0,0,0,0,0,                         // color map spec (ignored here)
		0,0,                               // x origin of image
		0,0,                               // y origin of image
		mWidth & 255,  mWidth >> 8 & 255,  // width of the image
		mHeight & 255, mHeight >> 8 & 255, // height of the image
		mPixelFormat << 3,                 // bits per pixel
		0                                  // image descriptor byte
	};

	// emit header, and pixel images
	fileStream.write(header, 18);
	fileStream.write(reinterpret_cast<const GLchar*>(mPixels),
	                 mWidth*mHeight*mPixelFormat);
	fileStream.close();
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Clear
void Tga::_Clear() {
	delete[] mPixels;
	mPixels = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Accessors
GLushort Tga::Width()     const {return mWidth;}
GLushort Tga::Height()    const {return mHeight;}
GLint Tga::PixelFormat()  const {return mPixelFormat;}
GLint Tga::BitsPerPixel() const {return 8;}
GLubyte* Tga::Pixels()    const {return mPixels;}


#ifndef _NO_PNG
////////////////////////////////////////////////////////////////////////////////
// Default constructor
Png::Png(): 
mPixels(NULL),
mPixelFormat(PIXEL_FORMAT_UNKNOWN),
mWidth(0), mHeight(0), 
mBitsPerPixel(0) {
}


////////////////////////////////////////////////////////////////////////////////
// Overloaded constructor
Png::Png(const std::string& filename) throw(FWException):
mPixels(NULL),
mPixelFormat(PIXEL_FORMAT_UNKNOWN),
mWidth(0), mHeight(0),
mBitsPerPixel(0) {
	Load(filename);
}


////////////////////////////////////////////////////////////////////////////////
// Clear
void Png::_Clear() {
	free(mPixels);
	mPixels = NULL;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
Png::~Png() {
	_Clear();
}


////////////////////////////////////////////////////////////////////////////////
// Load from file
void Png::Load(const std::string& filename) throw(FWException) {
	// Clear memory if necessary
	_Clear();

		// open file as binary
	FILE* fileStream = fopen(filename.c_str(), "rb");
	if(!fileStream) {
		throw _FileNotFoundException(filename);
	}

	// read and check header
	png_byte header[8];
	fread(header, 1, 8, fileStream);
	if(!png_check_sig(reinterpret_cast<png_byte*>(header), 8)) {
		fclose(fileStream);
		throw _PngInvalidHeaderException(filename);
	}

	// create png struct
	png_structp pngPtr = 
		png_create_read_struct(PNG_LIBPNG_VER_STRING, 
		                       NULL, NULL, NULL);
	if (!pngPtr) {
		fclose(fileStream);
		throw _PngCreateReadStructFailedException(filename);
	}

	// create png info struct
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) {
		png_destroy_read_struct(&pngPtr, (png_infopp) NULL, (png_infopp) NULL);
		fclose(fileStream);
		throw _PngCreateInfoStructFailedException(filename);
	}

	// create png info struct
	png_infop endPtr = png_create_info_struct(pngPtr);
	if (!endPtr) {
		png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp) NULL);
		fclose(fileStream);
		throw _PngCreateInfoStructFailedException(filename);
	}

	// png error stuff
	if (setjmp(png_jmpbuf(pngPtr))) {
		png_destroy_read_struct(&pngPtr, &infoPtr, &endPtr);
		fclose(fileStream);
		throw _PngSetJmpFailedException(filename);
	}

	// init png reading
	png_init_io(pngPtr, fileStream);

	// let libpng know you already read the first 8 bytes
	png_set_sig_bytes(pngPtr, 8);

	// read all the info up to the image data
	png_read_info(pngPtr, infoPtr);

	// get info about png
	GLint colourType(0), rowbytes(0), bpp(0);
	png_uint_32 w(0), h(0);
	png_get_IHDR(pngPtr,
	             infoPtr,
	             &w,
	             &h,
	             &bpp,
	             &colourType,
	             NULL, NULL, NULL);
	mWidth = w;
	mHeight = h;
	mBitsPerPixel = bpp;

	// set bit depth
	if(mBitsPerPixel!=8 && mBitsPerPixel!=16)
		throw _PngUnsupportedBitDepthException(filename);

	// set internalFormat and pixel format
	if(colourType==PNG_COLOR_TYPE_GRAY) {
		mPixelFormat = Png::PIXEL_FORMAT_LUMINANCE;
	}
	else if(colourType==PNG_COLOR_TYPE_GRAY_ALPHA) {
		mPixelFormat = Png::PIXEL_FORMAT_LUMINANCE_ALPHA;
	}
	else if(colourType==PNG_COLOR_TYPE_RGB) {
		mPixelFormat = Png::PIXEL_FORMAT_RGB;
	}
	else if(colourType==PNG_COLOR_TYPE_RGBA) {
		mPixelFormat = Png::PIXEL_FORMAT_RGBA;
	}
	else {
		png_destroy_read_struct(&pngPtr, &infoPtr, &endPtr);
		fclose(fileStream);
		throw _PngUnsupportedColourTypeException(filename);
	}

	// Update the png info struct.
	png_read_update_info(pngPtr, infoPtr);

	// Row size in bytes.
	rowbytes = png_get_rowbytes(pngPtr, infoPtr);

	// Allocate the image_data
	mPixels = reinterpret_cast<png_byte*>(malloc(rowbytes*mHeight));
	if(mPixels==NULL) {
		png_destroy_read_struct(&pngPtr, &infoPtr, &endPtr);
		fclose(fileStream);
		throw _MemoryAllocationFailedException();
	}

	// row_pointers is for pointing to image_data 
	// for reading the png with libpng
	png_bytepp rowPointers 
		= reinterpret_cast<png_bytepp>(malloc(rowbytes*sizeof(png_bytep)));
	if(rowPointers==NULL) {
		png_destroy_read_struct(&pngPtr, &infoPtr, &endPtr);
		fclose(fileStream);
		throw _MemoryAllocationFailedException();
	}

	// set the individual row_pointers to point at the correct offsets
	// of image_data
	for(png_uint_32 i = 0; i < mHeight; ++i)
		rowPointers[mHeight-1-i] = mPixels+i*rowbytes;

	// read the png into image_data through rowPointers
	png_read_image(pngPtr, rowPointers);

	// clean up
	png_destroy_read_struct(&pngPtr, &infoPtr, &endPtr);
	free(rowPointers);
	fclose(fileStream);
}


////////////////////////////////////////////////////////////////////////////////
// Accessors
GLushort Png::Width()     const {return mWidth;}
GLushort Png::Height()    const {return mHeight;}
GLint Png::PixelFormat()  const {return mPixelFormat;}
GLint Png::BitsPerPixel() const {return mBitsPerPixel;}
GLubyte* Png::Pixels()    const {return mPixels;}


#endif

} // namespace fw

