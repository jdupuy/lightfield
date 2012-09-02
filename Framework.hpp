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
	// Get exponent of next power of two
	GLuint next_power_of_two_exponent(GLuint number);


	// Build GLSL program
	GLvoid build_glsl_program(GLuint program, 
	                          const std::string& srcfile,
	                          const std::string& options,
	                          GLboolean link) throw(FWException);


	// Check OpenGL errors (uses ARB_debug_output if available)
	// (throws an exception if an error is detected)
	GLvoid check_gl_error() throw(FWException);
	GLvoid init_debug_output(std::ostream& outputStream) throw(FWException);


	// Save a portion of the OpenGL front/back buffer (= take a screenshot).
	// File will be a TGA in BGR format, uncompressed.
	// The OpenGL state is restored the way it was before this function call.
	GLvoid save_gl_front_buffer(GLint x,
	                            GLint y,
	                            GLsizei width,
	                            GLsizei height) throw(FWException);
	GLvoid save_gl_back_buffer(GLint x,
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


	// Upload a TGA in a bound 2D texture
	// immutable should be set to GL_FALSE on OpenGL3.3 (and older) hardware
	void tex_tga_image2D(const std::string& filename,
	                     GLboolean genMipmaps,
	                     GLboolean immutable) throw(FWException);
	// Upload 6 TGAs in a bound cubemap texture
	// immutable should be set to GL_FALSE on OpenGL3.3 (and older) hardware
	void tex_tga_cube_map(const std::string filenames[6],
	                      GLboolean genMipmaps,
	                      GLboolean immutable) throw(FWException);


	// Upload a PNG in a bound 2D texture (using libpng)
	// immutable should be set to GL_FALSE on OpenGL3.3 (and older) hardware
	void tex_png_image2D(const std::string& filename,
	                     GLboolean genMipmaps,
	                     GLboolean immutable) throw(FWException);
	// Upload 6 PNGs in a bound cubemap texture (using libpng)
	// filenames should specify each face separated by a comma in the following
	// order: positive_x, negative_x, positive_y, negative_y, 
	// positive_z, negative_z
	// immutable should be set to GL_FALSE on OpenGL3.3 (and older) hardware
	void tex_png_cube_map(const std::string filenames[6],
	                      GLboolean genMipmaps,
	                      GLboolean immutable) throw(FWException);


	// Render the frame using FSAA. Each pixel will be 
	// generated from the last mip level of a sampleCnt x sampleCnt 
	// texture. This is a heavy process which should be used
	// to generate ground truth images only.
	// - sampleCnt should be greater than 8 and preferably a power of two
	// - frustum gives the params of the projection (left, right, etc.)
	// - set_transform_func uploads the projection matrix to all the drawing
	// programs. 
	// - draw_func must perform all (and only all) the draw calls to the 
	// back buffer
	// Note: Requires an OpenGL4.3 GPU.
	void render_fsaa(GLsizei width, 
	                 GLsizei height,
	                 GLsizei sampleCnt,
	                 GLfloat *frustum, // frustum data
	                 GLboolean perspective, // perspective of ortho matrix
	                 void (*set_transforms_func)(float *perspectiveMatrix),
	                 void (*draw_func)() ) throw(FWException);


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
		enum {
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

