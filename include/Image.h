//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2014, Luke Goddard. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom
//  the Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////
#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <stdexcept>

inline int fromGamma22(double x)
{
	x = std::min( std::max( 0., x ), 1. );
	int v = int( pow( x, 1/2.2 ) * 255 + .5 );
	v = v > 255 ? 255 : v;
	v = v < 0 ? 0 : v;
	return v;
}

inline double toGamma22(int x)
{
	return pow( ( ( double( x ) ) / 255 ), 2.2 );
}

struct Image
{
	public :
	
		Image( int w = 1, int h = 1 );

		const double* readable( int x, int y ) const;
		double* writeable( int x, int y );
		const double* at( int x, int y ) const;
		inline int width() const { return m_width; };
		inline int height() const { return m_height; };
		
		inline void resize( int width, int height )
		{
			if( width < 1 || height < 1 )
			{
				throw std::runtime_error( "Cannot resize an image to null dimensions." );
			}

			m_width = width;
			m_height = height;
			m_data.resize( width * height * 3 );
		}

	private :
	
		int m_width, m_height;
		std::vector<double> m_data;
};

struct SampleSet
{
	public :

		SampleSet( const std::vector< Image > &i );

		inline int width() const { return m_width; };
		inline int height() const { return m_height; };
		inline const std::vector<double> &samples( int x, int y, int c ) const { return m_samples[ arrayIndex( x, y, c ) ]; }
		inline double mean( int x, int y, int c ) const { return m_mean[ arrayIndex( x, y, c ) ]; };
		inline double max( int x, int y, int c ) const { return m_max[ arrayIndex( x, y, c ) ]; };
		inline double min( int x, int y, int c ) const { return m_min[ arrayIndex( x, y, c ) ]; };
		inline double median( int x, int y, int c ) const { return m_median[ arrayIndex( x, y, c ) ]; };
		inline double variance( int x, int y, int c ) const { return m_variance[ arrayIndex( x, y, c ) ]; };
		inline double deviation( int x, int y, int c ) const { return m_deviation[ arrayIndex( x, y, c ) ]; };
		inline double midpoint( int x, int y, int c ) const
		{
			double mn = min( x, y, c );
			double mx = max( x, y, c );
			return ( mx - mn ) * .5 + mn;
		 };

	private :

		inline int arrayIndex( int x, int y, int c ) const
		{ 
			x = std::max( std::min( x, m_width - 1 ), 0 );
			y = std::max( std::min( y, m_height - 1 ), 0 );
			c = std::max( std::min( c, 3 ), 0 );
			return ( y * m_width + x ) * 3 + c;
		}

		int m_width, m_height;
		std::vector< std::vector< double > > m_samples;
		std::vector< double > m_mean, m_variance, m_deviation, m_min, m_max, m_median;
};

struct BmpHeader
{
	unsigned int   mFileSize;        // Size of file in bytes
	unsigned int   mReserved01;      // 2x 2 reserved bytes
	unsigned int   mDataOffset;      // Offset in bytes where data can be found (54)

	unsigned int   mHeaderSize;      // 40B
	int    mWidth;           // Width in pixels
	int    mHeight;          // Height in pixels

	short  mColorPlates;     // Must be 1
	short  mBitsPerPixel;    // We use 24bpp
	unsigned int   mCompression;     // We use BI_RGB ~ 0, uncompressed
	unsigned int   mImageSize;       // mWidth x mHeight x 3B
	unsigned int   mHorizRes;        // Pixels per meter (75dpi ~ 2953ppm)
	unsigned int   mVertRes;         // Pixels per meter (75dpi ~ 2953ppm)
	unsigned int   mPaletteColors;   // Not using palette - 0
	unsigned int   mImportantColors; // 0 - all are important
};

bool readPPM( const std::string &path, Image &image );
bool writePPM( const std::string &path, const Image &image );
bool writeBMP( const std::string &path, const Image &image );

#endif
