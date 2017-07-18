/***********************************************************************
TextureSet - Class to manage a set of RGB images as OpenGL textures.
Copyright (c) 2016 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IMAGES_TEXTURESET_INCLUDED
#define IMAGES_TEXTURESET_INCLUDED

#include <vector>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Images/BaseImage.h>

namespace Images {

class TextureSet:public GLObject
	{
	/* Embedded classes: */
	public:
	typedef unsigned int Key; // Type for keys to uniquely identify textures in the set
	
	class Texture // Structure to represent a single texture image
		{
		friend class TextureSet;
		
		/* Elements: */
		private:
		Key key; // Unique key to access this texture
		Images::BaseImage image; // The texture's source image
		GLenum target; // Texture target in which to cache the source image
		GLenum internalFormat; // Internal texture format for the source image
		unsigned int imageVersion; // Version number of the source image to invalidate cached textures
		GLint mipmapRange[2]; // Minimum and maximum mipmap levels
		GLenum wrapModes[2]; // Horizontal and vertical texture wrapping modes
		GLenum filterModes[2]; // Minification and magnification texture filtering modes
		unsigned int settingsVersion; // Version number of texture settings to invalidate cached settings
		
		/* Methods: */
		public:
		Key getKey(void) const // Returns the texture's key
			{
			return key;
			}
		const Images::BaseImage& getImage(void) const // Returns the texture's source image
			{
			return image;
			}
		Images::BaseImage& getImage(void) // Ditto
			{
			return image;
			}
		void updateImage(void) // Notifies the texture that the image was changed
			{
			/* Invalidate the cached texture image: */
			++imageVersion;
			}
		void setImage(const Images::BaseImage& newImage) // Sets the texture image to the provided source image
			{
			/* Assign the image and invalidate the cached texture: */
			image=newImage;
			++imageVersion;
			}
		GLenum getTarget(void) const // Returns the image's assigned texture target
			{
			return target;
			}
		GLenum getInternalFormat(void) const // Returns the image's internal texture format
			{
			return internalFormat;
			}
		const GLint* getMipmapRange(void) const // Returns the mipmap level range
			{
			return mipmapRange;
			}
		const GLenum* getWrapModes(void) const // Returns the image's texture wrapping modes
			{
			return wrapModes;
			}
		const GLenum* getFilterModes(void) const // Returns the image's texture filtering modes
			{
			return filterModes;
			}
		void setMipmapRange(GLint baseLevel,GLint maxLevel) // Sets the image's mipmap level range; maxLevel larger than zero enables automatic mipmap generation
			{
			/* Update the mipmap range: */
			mipmapRange[0]=baseLevel;
			mipmapRange[1]=maxLevel;
			
			/* Invalidate the cached texture settings and the cached image version to trigger mipmap generation: */
			++imageVersion;
			++settingsVersion;
			}
		void setWrapModes(GLenum newWrapS,GLenum newWrapT) // Sets the image's texture wrapping modes
			{
			/* Update the wrapping modes: */
			wrapModes[0]=newWrapS;
			wrapModes[1]=newWrapT;
			
			/* Invalidate the cached texture settings: */
			++settingsVersion;
			}
		void setFilterModes(GLenum newMinFilter,GLenum newMagFilter) // Sets the image's texture filtering modes
			{
			/* Update the filtering modes: */
			filterModes[0]=newMinFilter;
			filterModes[1]=newMagFilter;
			
			/* Invalidate the cached texture settings: */
			++settingsVersion;
			}
		};
	
	private:
	typedef std::vector<Texture> TextureList; // Type for lists of textures
	
	public:
	class GLState:public GLObject::DataItem // Structure holding per-context OpenGL state
		{
		friend class TextureSet;
		
		/* Embedded classes: */
		public:
		class Texture // Structure to represent a single texture image in an OpenGL context
			{
			friend class TextureSet;
			friend class GLState;
			
			/* Elements: */
			private:
			Key key; // Unique key to associate application textures with OpenGL textures
			const BaseImage* image; // Reference to the source image
			GLuint textureObjectId; // ID of OpenGL texture object allocated for this texture
			GLfloat texCoordMin[2],texCoordMax[2]; // Texture coordinate range to map the entire texture to a surface
			unsigned int imageVersion; // Version number of image currently cached in texture object
			unsigned int settingsVersion; // Version number of texture settings currently cached in texture object
			
			/* Constructors and destructors: */
			Texture(Key sKey,const BaseImage* sImage)
				:key(sKey),image(sImage),
				 imageVersion(0U),settingsVersion(0U)
				{
				}
			
			/* Methods: */
			public:
			const BaseImage& getImage(void) const // Returns the texture's source image
				{
				return *image;
				}
			const GLfloat* getTexCoordMin(void) const // Returns the lower-left texture coordinate
				{
				return texCoordMin;
				}
			const GLfloat* getTexCoordMax(void) const // Returns the upper-right texture coordinate
				{
				return texCoordMax;
				}
			};
		
		/* Elements: */
		private:
		const TextureSet& textureSet; // Reference to texture set to which this GL state belongs
		bool haveNpotdTextures; // Flag whether OpenGL supports non-power-of-two-dimension textures
		bool haveGenerateMipmap; // Flag whether OpenGL supports automatic mipmap generation
		std::vector<Texture> textures; // The set of managed texture objects, sorted by key
		unsigned int textureSetVersion; // Version number of the set of texture objects
		
		/* Private methods: */
		void insertTexture(unsigned int pos,const TextureSet::Texture& source,GLuint textureObjectId =0U); // Inserts a new texture based on the given source texture into the texture set before the given position
		void removeTexture(unsigned int pos); // Removes the texture at the given position from the texture set
		
		/* Constructors and destructors: */
		private:
		GLState(const TextureSet& sTextureSet); // Creates per-context state for given texture set
		public:
		virtual ~GLState(void); // Releases all allocated resources
		
		/* New methods: */
		const Texture& bindTexture(Key key); // Binds the texture object associated with the given key to its texture target on the current texture unit and returns texture state; throws exception if key does is not in the texture set
		};
	
	friend class GLState;
	
	/* Elements: */
	TextureList textures; // The set of managed texture images, sorted by key
	unsigned int textureSetVersion; // Version number of the set of texture images
	
	/* Constructors and destructors: */
	public:
	TextureSet(void); // Creates an empty texture set
	virtual ~TextureSet(void); // Destroys the texture set
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	Texture& addTexture(const Images::BaseImage& newImage,GLenum newTarget,GLenum newInternalFormat); // Adds a new texture image to the set and returns the new texture set entry
	Texture& addTexture(const Images::BaseImage& newImage,GLenum newTarget,GLenum newInternalFormat,Key newKey); // Adds a new texture image for the given key and returns the new texture set entry; throws exception if key is not unique
	const Texture& getTexture(Key key) const; // Returns texture set item associated with the given key; throws exception if key is not in texture set
	Texture& getTexture(Key key); // Ditto
	void deleteTexture(Key key); // Deletes the texture associated with the given key; throws exception if key is not in texture set
	GLState* getGLState(GLContextData& contextData) const; // Returns OpenGL texture state object for the given OpenGL context
	};

}

#endif
