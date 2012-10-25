////////////////////////////////////////////////////////////////////////////////
// \author J Dupuy
// \brief Utility functions and classes for simple OpenGL demos.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef FRAMEWORK_HPP
#define FRAMEWORK_HPP

#include <string>
#include <vector>
#include "glew.hpp"

// offset for buffer objects
#define FW_BUFFER_OFFSET(i)    ((char*)NULL + (i))

namespace fw {
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


	// Check OpenGL errors
	// (throws an exception if an error is detected)
	GLvoid check_gl_error() throw(FWException);


	// Check framebuffer status
	// (throws an exception if the active framebuffer is not framebuffer complete)
	GLvoid check_framebuffer_status() throw(FWException);


	// Init debug output
	// (throws an exception if the extension is not supported)
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


	// Half to float conversion
	GLhalf float_to_half(GLfloat f);
	GLfloat half_to_float(GLhalf h);


	// Pack four normalized floats in an unsigned integer using
	// equation 2.1 from August 6, 2012 GL4.3 core profile specs.
	// The values must be in range [0.f,1.f] (checked in debug mode)
	// Memory layout: msb                                 lsb
	//                wwzz zzzz zzzz yyyy yyyy yyxx xxxx xxxx
	GLuint pack_4f_to_uint_2_10_10_10_rev(GLfloat x,
	                                      GLfloat y,
	                                      GLfloat z,
	                                      GLfloat w);
	GLuint pack_4fv_to_uint_2_10_10_10_rev(const GLfloat *v);


	// Pack four normalized floats in a signed integer using
	// equation 2.2 from August 6, 2012 GL4.3 core profile specs.
	// The values must be in range [-1.f,1.f] (checked in debug mode)
	// Memory layout: msb                                 lsb
	//                wwzz zzzz zzzz yyyy yyyy yyxx xxxx xxxx
	GLint pack_4f_to_int_2_10_10_10_rev(GLfloat x,
	                                    GLfloat y,
	                                    GLfloat z,
	                                    GLfloat w);
	GLint pack_4fv_to_int_2_10_10_10_rev(const GLfloat *v);


	// Convert RGB8 to packed types.
	GLubyte pack_3ub_to_ubyte_3_3_2(GLubyte r,
	                                GLubyte g,
	                                GLubyte b);
	GLushort pack_3ub_to_ushort_4_4_4(GLubyte r,
	                                  GLubyte g,
	                                  GLubyte b);
	GLushort pack_3ub_to_ushort_5_5_5(GLubyte r,
	                                  GLubyte g,
	                                  GLubyte b);
	GLushort pack_3ub_to_ushort_5_6_5(GLubyte r,
	                                  GLubyte g,
	                                  GLubyte b);
	GLubyte pack_3ubv_to_ubyte_3_3_2(const GLubyte *v);
	GLushort pack_3ubv_to_ushort_4_4_4(const GLubyte *v);
	GLushort pack_3ubv_to_ushort_5_5_5(const GLubyte *v);
	GLushort pack_3ubv_to_ushort_5_6_5(const GLubyte *v);


	// Convert RGBA8 to packed types.
	GLushort pack_4ub_to_ushort_4_4_4_4(GLubyte r,
	                                    GLubyte g,
	                                    GLubyte b,
	                                    GLubyte a);
	GLushort pack_4ub_to_ushort_5_5_5_1(GLubyte r,
	                                    GLubyte g,
	                                    GLubyte b,
	                                    GLubyte a);
	GLushort pack_4ubv_to_ushort_4_4_4_4(const GLubyte *v);
	GLushort pack_4ubv_to_ushort_5_5_5_1(const GLubyte *v);


	// Upload a TGA to a texture bound as GL_TEXTURE_2D
	void tex_tga_image2D(const std::string& filename,
	                     GLboolean genMipmaps,
	                     GLboolean immutable) throw(FWException);
	// Upload 6 TGAs to a texture bound as GL_TEXTURE_CUBE_MAP
	void tex_tga_cube_map(const std::string filenames[6],
	                      GLboolean genMipmaps,
	                      GLboolean immutable) throw(FWException);
	// Upload multiple TGAs to a texture bound as GL_TEXTURE_3D
	void tex_tga_sprites_image3D(const std::vector<std::string>& filenames,
	                             GLboolean genMipmaps,
	                             GLboolean immutable) throw(FWException);

#ifndef _NO_PNG // removes dependencies on libpng
	// Upload a PNG to a texture bound as GL_TEXTURE_2D (using libpng)
	void tex_png_image2D(const std::string& filename,
	                     GLboolean genMipmaps,
	                     GLboolean immutable) throw(FWException);
	// Upload 6 PNGs to a texture bound as GL_TEXTURE_CUBE_MAP (using libpng)
	void tex_png_cube_map(const std::string filenames[6],
	                      GLboolean genMipmaps,
	                      GLboolean immutable) throw(FWException);
	// Upload multiple PNGs to a texture bound as GL_TEXTURE_3D (using libpng)
	void tex_png_sprites_image3D(const std::vector<std::string>& filenames,
	                             GLboolean genMipmaps,
	                             GLboolean immutable) throw(FWException);
#endif // _NO_PNG


	// Build meshes centered at (0,0,0).
	// Three buffers must be bound: GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER
	// and GL_DRAW_INDIRECT_BUFFER.
	// The vertex array must be built by the user, who also provides the 
	// Vertex type which must have the following constructor:
	// Vertex(GLfloat px, GLfloat py, GLfloat pz,
	//        GLfloat nx, GLfloat ny, GLfloat nz,
	//        GLfloat tx, GLfloat ty, GLfloat tz);
	// Index format is unsigned short, if the amount of indexes exceeds is 
	// uint16 limit, an exception is thrown.
	template<class Vertex>
	void buffer_cylinder_data(GLfloat base,
	                          GLfloat top,
	                          GLfloat height,
	                          GLint slices,
	                          GLint stacks) throw(FWException);

	template<class Vertex>
	void buffer_sphere_data(GLfloat radius,
	                        GLint slices,
	                        GLint stacks) throw(FWException);

	template<class Vertex>
	void buffer_disk_data(GLfloat inner,
	                      GLfloat outer,
	                      GLint slices,
	                      GLint loops) throw(FWException);

	template<class Vertex>
	void buffer_cube_data(GLfloat width,
	                      GLfloat height,
	                      GLfloat depth) throw(FWException);


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
	                 void (*set_transforms_func)(GLfloat *perspectiveMatrix),
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
		GLint    BitsPerPixel() const;
		GLubyte* Pixels()      const; // data must be used for read only

	private:
		// Non copyable
		Tga(const Tga& tga);
		Tga& operator=(const Tga& tga);

		// Internal manipulation
		GLushort _UnpackUint16(GLubyte msb, GLubyte lsb);
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
		GLint    mPixelFormat;
		GLushort mWidth;
		GLushort mHeight;
	};

#ifndef _NO_PNG // removes dependencies on libpng
	class Png {
	public:
		// Constants
		enum {
			PIXEL_FORMAT_UNKNOWN=0,
			PIXEL_FORMAT_LUMINANCE,
			PIXEL_FORMAT_LUMINANCE_ALPHA,
			PIXEL_FORMAT_RGB,
			PIXEL_FORMAT_RGBA
		};

		// Constructors / Destructor
		Png();
			// see Load
		explicit Png(const std::string& filename) throw(FWException);
		~Png();

		// Manipulation
			// load from a tga file
		void Load(const std::string& filename) throw(FWException);

		// Queries
		GLushort Width()        const;
		GLushort Height()       const;
		GLint    PixelFormat()  const;
		GLint    BitsPerPixel() const;
		GLubyte* Pixels()       const; // data must be used for read only

	private:
		// Non copyable
		Png(const Png& png);
		Png& operator=(const Png& png);

		// Internal manipulation
		void _Clear();

		// Members
		GLubyte* mPixels;
		GLint    mPixelFormat;
		GLushort mWidth;
		GLushort mHeight;
		GLubyte  mBitsPerPixel;
	};
#endif


} // namespace fw


#include "Framework.inl" // mesh impl

#endif

