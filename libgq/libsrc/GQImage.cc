
#include <GQImage.h>
#include <GQInclude.h>
#include <string.h>
#include <stdio.h>

#include <QImage>
#include <QColor>
#include <QtGui>

inline float clamp( float f, float min, float max )
{
    if (f > max)
        return max;
    else if (f < min)
        return min;
    else
        return f;
}

void GQImage::clear()
{
    _width = _height = _num_chan = 0u;
    if (_raster)
    {
        delete [] _raster;
        _raster = NULL;
    }
}

GQImage::GQImage() 
{ 
    _width = _height = _num_chan = 0; 
    _raster = NULL; 
}

GQImage::GQImage(int w, int h, int c)
{
    _width    = w;
    _num_chan = c;
    _height   = h;
    _raster   = new uint8[w*h*c];

    if (!_raster)
    {
        fprintf(stderr,"ERROR - GQImage::GQImage - failed to alloc _raster\n");
        exit(1);
    }
}

bool GQImage::resize(int w, int h, int c)
{
    if (likely( w*h*c == _width*_height*_num_chan ))
    {
        _width    = w;
        _height   = h;
        _num_chan = c;
        return true;
    }

    _width    = w;
    _height   = h;
    _num_chan = c;

    if (_raster) delete [] _raster;
    _raster = new uint8[w*h*c];

    if (!_raster)
    {
        clear();
        return false;
    }
    return true; 
}

bool GQImage::save( const QString& filename, bool flip)
{
	QImage qi( _width, _height, QImage::Format_ARGB32 );

	if (_num_chan == 3)
	{
		for (int i = 0; i < _width*_height; i++)
			qi.setPixel( i%_width, i/_width, qRgba(_raster[3*i], _raster[3*i + 1], _raster[3*i + 2], 255) );
	}
	else if (_num_chan == 4)
	{
		for (int i = 0; i < _width*_height; i++)
			qi.setPixel( i%_width, i/_width, qRgba(_raster[4*i], _raster[4*i + 1], _raster[4*i + 2], _raster[4*i + 3]) );
	}
	else if (_num_chan == 1)
	{
		for (int i = 0; i < _width*_height; i++)
			qi.setPixel( i%_width, i/_width, qRgba(_raster[i], _raster[i], _raster[i], 255) );
	}
	else
	{
		qWarning("GQImage::save: unsupported format (%s).\n", qPrintable(filename));
		return false;
	}

	if (flip)
		qi = qi.mirrored();

	return qi.save( filename );
}

bool GQImage::load(const QString& filename)
{
	QImage qi;
	if (qi.load(filename))
	{
		if (qi.hasAlphaChannel())
		{
			resize(qi.width(), qi.height(), 4);
			for (int i = 0; i < _width*_height; i++)
			{
				QRgb pix = qi.pixel(i%_width, qi.height() - i/_width - 1);
				_raster[4*i  ] = qRed(pix);
				_raster[4*i+1] = qGreen(pix);
				_raster[4*i+2] = qBlue(pix);
				_raster[4*i+3] = qAlpha(pix);
			}
		}
		else if (qi.isGrayscale())
		{
			resize(qi.width(), qi.height(), 1);
			for (int i = 0; i < _width*_height; i++)
			{
				QRgb pix = qi.pixel(i%_width, qi.height() - i/_width - 1);
				_raster[i] = qGray(pix);
			}		
		}
		else
		{
			resize(qi.width(), qi.height(), 3);
			for (int i = 0; i < _width*_height; i++)
			{
				QRgb pix = qi.pixel(i%_width, qi.height() - i/_width - 1);
				_raster[3*i  ] = qRed(pix);
				_raster[3*i+1] = qGreen(pix);
				_raster[3*i+2] = qBlue(pix);
			}
		}
		return true;
	}
	return false;
}

GQFloatImage::GQFloatImage() 
{ 
    _width = _height = _num_chan = 0; 
    _raster = NULL; 
}

void GQFloatImage::clear()
{
    _width = _height = _num_chan = 0u;
    if (_raster)
    {
        delete [] _raster;
        _raster = NULL;
    }
}

GQFloatImage::GQFloatImage(int w, int h, int c)
{
    _width    = w;
    _num_chan = c;
    _height   = h;
    _raster   = new float[w*h*c];

    if (!_raster)
    {
        fprintf(stderr,"ERROR - GQFloatImage::GQFloatImage - failed to alloc raster\n");
        exit(1);
    }
}

bool GQFloatImage::resize(int w, int h, int c)
{
    if (likely( w*h*c == _width*_height*_num_chan ))
    {
        _width    = w;
        _height   = h;
        _num_chan = c;
        return true;
    }

    _width    = w;
    _height   = h;
    _num_chan = c;

    if (_raster) delete [] _raster;
    _raster = new float[w*h*c];

    if (!_raster)
    {
        clear();
        return false;
    }
    return true; 
}

void GQFloatImage::scaleValues( float factor )
{
    for (int i = 0; i < _width*_height*_num_chan; i++)
	{
        _raster[i] *= factor;
	}
}

void GQFloatImage::setPixel( int x, int y, const float* pixel )
{
	for (int i = 0; i < _num_chan; i++)
		_raster[_num_chan * (x + y*_width) + i] = pixel[i];
}

void GQFloatImage::setPixelChannel( int x, int y, int c, float value )
{
	_raster[_num_chan * (x + y*_width) + c] = value;
}


bool GQFloatImage::save(const QString& filename, bool flip /* = true */ )
{
    if (filename.endsWith("pfm"))
        return savePFM(filename, flip);
    else if (filename.endsWith("float"))
        return saveFloat(filename, flip);
    else
        return saveQImage(filename, flip);
}

bool GQFloatImage::saveQImage( const QString& filename, bool flip)
{
    QImage qi( _width, _height, QImage::Format_ARGB32 );

	if (_num_chan == 3)
	{
		for (int i = 0; i < _width*_height; i++)
        {
            uint8 r = clamp(_raster[3*i], 0.0f, 255.0f);
            uint8 g = clamp(_raster[3*i+1], 0.0f, 255.0f);
            uint8 b = clamp(_raster[3*i+2], 0.0f, 255.0f);
			qi.setPixel( i%_width, i/_width, qRgba(r, g, b, 255) );
        }
	}
	else if (_num_chan == 4)
	{
		for (int i = 0; i < _width*_height; i++)
        {
            uint8 r = clamp(_raster[4*i], 0.0f, 255.0f);
            uint8 g = clamp(_raster[4*i+1], 0.0f, 255.0f);
            uint8 b = clamp(_raster[4*i+2], 0.0f, 255.0f);
            uint8 a = clamp(_raster[4*i+3], 0.0f, 255.0f);
			qi.setPixel( i%_width, i/_width, qRgba(r, g, b, a) );
        }
	}
    else if (_num_chan == 1)
    {
		for (int i = 0; i < _width*_height; i++)
        {
            uint8 r, g, b;
            uint8 grey = clamp(_raster[i], 0.0f, 255.0f);
            r = g = b = grey;
			qi.setPixel( i%_width, i/_width, qRgba(r, g, b, 255) );
        }
    }
	else
	{
		qWarning("GQFloatImage::save: unsupported format (%s).\n", qPrintable(filename));
		return false;
	}

	if (flip)
		qi = qi.mirrored();

	return qi.save( filename );
}

bool GQFloatImage::saveFloat(const QString& filename, bool flip)
{
    Q_UNUSED(flip);

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    int dim[3];
    dim[0] = width();
    dim[1] = height();
    dim[2] = chan();
    file.write((const char*)&dim[0], sizeof(dim));
    file.write((const char*)raster(), dim[0]*dim[1]*dim[2]*sizeof(float));

    file.close();

    return true;
}

static bool we_are_little_endian()
{
    char buf[4];
    *(int *)(&buf[0]) = 1;
    return (buf[0] == 1);
}

bool GQFloatImage::savePFM(const QString& filename, bool flip)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QString version = "Pf";
    if (chan() > 1)
        version = "PF";
    QString header;
    header.sprintf("%s\n%d %d\n%.1f\n", qPrintable(version), width(), height(), 
                   we_are_little_endian() ? -1.0f : 1.0f).toAscii();
    file.write(header.toAscii(), header.toAscii().size());

	int write_chan = std::min(chan(), 3);
	for (int y = 0; y < height(); y++)
	{
		int yy = y;
		if (flip)
			yy = height() - y - 1;

		for (int x = 0; x < width(); x++)
		{
			file.write((const char*)(&raster()[chan()*(x + yy*width())]),
					   write_chan*sizeof(float));
		}
	}

    file.close();
    return true;
}

bool GQFloatImage::loadPFM(const QString& filename)
{
	QFile inputFile( filename );

	// try to open the file in read only mode
	if( !( inputFile.open( QIODevice::ReadOnly ) ) )
	{		
		return false;
	}

	QTextStream inputTextStream( &inputFile );
	inputTextStream.setCodec( "ISO-8859-1" );

	// read header
	QString qsType;
	QString qsWidth;
	QString qsHeight;
	QString qsScale;	

	int width;
	int height;
	int channels;
	float scale;

	inputTextStream >> qsType;
	if (qsType == "PF")
	{
		channels = 3;
	}
	else if (qsType == "Pf")
	{
		channels = 1;
	}
	else
	{
		inputFile.close();
		return false;
	}

	inputTextStream >> qsWidth >> qsHeight >> qsScale;

	width = qsWidth.toInt();
	height = qsHeight.toInt();
	scale = qsScale.toFloat();

	if( width < 0 || height < 0 || scale >= 0 )
	{
		inputFile.close();
		return false;
	}

	// close the text stream
	inputTextStream.setDevice( NULL );
	inputFile.close();

	// now reopen it again in binary mode
	if( !( inputFile.open( QIODevice::ReadOnly ) ) )
	{
		return false;
	}

	int headerLength = qsType.length() + qsWidth.length() + 
                       qsHeight.length() + qsScale.length() + 4;	

	QDataStream inputDataStream( &inputFile );
	inputDataStream.skipRawData( headerLength );

	resize(width, height, channels);
	float buffer[3];

	for( int y = 0; y < height; ++y )
	{
		for( int x = 0; x < width; ++x )
		{
			int yy = height - y - 1;

			inputDataStream.readRawData( 
                    reinterpret_cast< char* >( &( buffer[ 0 ] ) ), 
                    channels * sizeof( float ) );
			for (int c = 0; c < channels; c++)
				setPixelChannel(x, yy, c, buffer[c]);
		}
	}

	inputFile.close();
	return true;
}

bool GQFloatImage::load(const QString& filename)
{
    GQImage img;
	if (filename.endsWith(".pfm"))
	{
		return loadPFM(filename);
	}
	else
	{
	    bool ret = img.load(filename);
	    if (ret) {
	        resize(img.width(), img.height(), img.chan());
	        for (int i = 0; i < img.width()*img.height()*img.chan(); i++)
	            raster()[i] = (float)(img.raster()[i]) / 255.0f;

	        return true;
	    }
	}
    
    return false;
}
