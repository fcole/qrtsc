/*****************************************************************************\

GQImage.h
Authors: Forrester Cole (fcole@cs.princeton.edu)
         R. Keith Morley
         Adam Finkelstein
Copyright (c) 2009 Forrester Cole

Simple 8bit image. Uses QImage for I/O.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _GQ_IMAGE_H_
#define _GQ_IMAGE_H_

#include <QString>

class GQImage
{
public:
    GQImage(); 
    GQImage(int w, int h, int c);
    ~GQImage()      { clear(); }

    int width()const      { return _width; }
    int height()const      { return _height; }
    int chan()const   { return _num_chan; }
    unsigned char* raster() { return _raster; } 
    const unsigned char* raster() const { return _raster; } 
	unsigned char pixel( int x, int y, int c ) const 
        { return _raster[_num_chan * (x + y*_width) + c]; }
	
	void setPixelChannel( int x, int y, int c, unsigned char value );
   
    bool resize(int w, int h, int c);
    void copy( const GQImage& from )
    {
        resize( from._width, from._height, from._num_chan );
        memcpy( _raster, from._raster, _width*_height*_num_chan );
    }

    void clear();

	bool save(const QString& filename, bool flip = true );
    bool load(const QString& filename);

private:
    int _width;
    int _height;
    int _num_chan;
    unsigned char* _raster;
};

// This really should be a templated version of above, but QImage 
// doesn't work for floating point image I/O.
class GQFloatImage
{
public:
    GQFloatImage(); 
    GQFloatImage(int w, int h, int c);
    ~GQFloatImage()      { clear(); }

    int width()const      { return _width; }
    int height()const      { return _height; }
    int chan()const   { return _num_chan; }
    float* raster() { return _raster; } 
    const float* raster() const { return _raster; } 
	float  pixel( int x, int y, int c ) const 
        { return _raster[_num_chan * (x + y*_width) + c]; }
   
	void setPixel( int x, int y, const float* pixel );
	void setPixelChannel( int x, int y, int c, float value );

    bool resize(int w, int h, int c);
    void scaleValues( float factor );
    void copy( const GQFloatImage& from )
    {
        resize( from._width, from._height, from._num_chan );
        memcpy( _raster, from._raster, _width*_height*_num_chan );
    }

    void clear();

	bool save(const QString& filename, bool flip = true );
    bool load(const QString& filename);

protected:
    bool saveFloat(const QString& filename, bool flip);
    bool savePFM(const QString& filename, bool flip);
    bool saveQImage(const QString& filename, bool flip);

	bool loadPFM(const QString& filename);

private:
    int _width;
    int _height;
    int _num_chan;
    float* _raster;
};


#endif //_GQ_IMAGE_H_




