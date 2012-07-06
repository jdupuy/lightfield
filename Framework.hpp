////////////////////////////////////////////////////////////////////////////////
// \author J Dupuy
// \brief Utility functions and classes for simple OpenGL demos.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef FRAMEWORK_HPP
#define FRAMEWORK_HPP

#include <string>
#include "glew.hpp"

// offset for buffer objects
#define FW_BUFFER_OFFSET(i)    ((char*)NULL + (i))

namespace fw 
{
	// Framework exception
	class FWException : public std::exception {
	public:
		virtual ~FWException()   throw()   {}
		const char* what() const throw()   {return mMessage.c_str();}
	protected:
		std::string mMessage;
	};


	// Check if power of two
	bool is_power_of_two(GLuint number);


	// Get next power of two
	GLuint next_power_of_two(GLuint number);


	// Build GLSL program
	GLvoid build_glsl_program(GLuint program, 
	                          const std::string& srcfile,
	                          const std::string& options,
	                          GLboolean link) throw(FWException);


	// Check OpenGL errors (uses ARB_debug_output if available)
	// (throws an exception if an error is detected)
	GLvoid check_gl_error() throw(FWException);


	// Save a portion of the OpenGL front buffer (= take a screenshot).
	// File will be a TGA in BGR format, uncompressed.
	// The OpenGL state is restored the way it was before this function call.
	GLvoid save_gl_front_buffer(GLint x,
	                            GLint y,
	                            GLsizei width,
	                            GLsizei height) throw(FWException);


	// Pack four floats in an unsigned integer
	// Floats are clamped in range [0,1]
	// (follows GL4.2 specs)
	GLuint pack_4f_to_uint_10_10_10_2(GLfloat x,
	                                  GLfloat y,
	                                  GLfloat z,
	                                  GLfloat w);
	GLuint pack_4fv_to_uint_10_10_10_2(const GLfloat *v);


	// Pack four floats in a signed integer
	// Floats are clamped in range [-1,1]
	// (follows GL4.2 specs)
	GLint pack_4f_to_int_10_10_10_2(GLfloat x,
	                                GLfloat y,
	                                GLfloat z,
	                                GLfloat w);
	GLint pack_4fv_to_int_10_10_10_2(const GLfloat *v);


	// Half to float conversion
	GLhalf float_to_half(GLfloat f);
	GLfloat half_to_float(GLhalf h);

	// Upload a TGA in a 2D texture
	// immutable should be set to GL_FALSE on OpenGL3.3 (and older) hardware
	void tex_tga_image2D(const std::string& filename,
	                     GLuint texture,
	                     GLboolean genMipmaps,
	                     GLboolean immutable) throw(FWException);

	// Indirect drawing command : DrawArraysIndirectCommand
	typedef struct {
		GLuint count;
		GLuint primCount;
		GLuint first;
		GLuint baseInstance;
	} DrawArraysIndirectCommand;


	// Indirect drawing command : DrawElementsIndirectCommand
	typedef struct {
		GLuint count;
		GLuint primCount;
		GLuint firstIndex;
		GLint baseVertex;
		GLuint baseInstance;
	} DrawElementsIndirectCommand;


	// Basic timer class
	class Timer {
	public:
		// Constructors / Destructor
		Timer();

		// Manipulation
		void Start();
		void Stop() ;

		// Queries
		double Ticks()   const;

		// Members
	private:
		double mStartTicks;
		double mStopTicks;
		bool   mIsTicking;
	};


	// Tga image loader
	class Tga {
	public:
		// Constants
		enum
		{
			PIXEL_FORMAT_UNKNOWN=0,
			PIXEL_FORMAT_LUMINANCE,
			PIXEL_FORMAT_LUMINANCE_ALPHA,
			PIXEL_FORMAT_BGR,
			PIXEL_FORMAT_BGRA
		};

		// Constructors / Destructor
		Tga();
			// see Load
		explicit Tga(const std::string& filename) throw(FWException);
		~Tga();

		// Manipulation
			// load from a tga file
		void Load(const std::string& filename) throw(FWException);

		// Queries
		GLushort Width()       const;
		GLushort Height()      const;
		GLint    PixelFormat() const;
		GLubyte* Pixels()      const; // data must be used for read only

	private:
		// Non copyable
		Tga(const Tga& tga);
		Tga& operator=(const Tga& tga);

		// Internal manipulation
		void _Flip();
		void _LoadColourMapped(std::ifstream&, GLchar*)    throw(FWException);
		void _LoadLuminance(std::ifstream&, GLchar*)       throw(FWException);
		void _LoadUnmapped(std::ifstream&, GLchar*)        throw(FWException);
		void _LoadUnmappedRle(std::ifstream&, GLchar*)     throw(FWException);
		void _LoadColourMappedRle(std::ifstream&, GLchar*) throw(FWException);
		void _LoadLuminanceRle(std::ifstream&, GLchar*)    throw(FWException);
		void _Clear();

		// Members
		GLubyte* mPixels;
		GLushort mWidth;
		GLushort mHeight;
		GLint    mPixelFormat;
	};


} // namespace fw

#endif

