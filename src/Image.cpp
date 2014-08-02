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
#include <limits>
#include <stdexcept>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <fstream>

#include "Image.h"

Image::Image( int w, int h )
{
	resize( w, h );
}

const double* Image::readable( int x, int y ) const
{
	x = std::max( std::min( x, m_width - 1 ), 0 );
	y = std::max( std::min( y, m_height - 1 ), 0 );
	return &m_data[ ( x + m_width * y ) * 3 ];
}

double* Image::writeable( int x, int y )
{
	return &m_data[ ( x + m_width * y ) * 3 ];
}

const double* Image::at( int x, int y ) const
{
	return &m_data[ ( x + m_width * y ) * 3 ];
}

SampleSet::SampleSet( const std::vector< Image > &images ) :
	m_width(0),
	m_height(0)
{
	for( unsigned int j = 0; j < images.size(); ++j )
	{
		if( m_width == 0 && m_height == 0 )
		{
			m_width = images[j].width();
			m_height = images[j].height();
		}
		else
		{
			if( m_width != images[j].width() || m_height != images[j].height() )
			{
				throw std::runtime_error( "Not all images are the same size." );
			}
		}
	}
	m_samples.resize( m_width * m_height * 3 );

	const int arraySize = m_width * m_height * 3;	
	m_mean.resize( arraySize );
	m_median.resize( arraySize );
	m_variance.resize( arraySize );
	m_deviation.resize( arraySize );
	m_min.resize( arraySize );
	m_max.resize( arraySize );

	int index = 0;
	std::vector<double> s;
	for( int y = 0; y < m_height; ++y )
	{
		for( int x = 0; x < m_width; ++x )
		{
			for( int c = 0; c < 3; ++c, ++index )
			{
				s.resize( images.size() );
				for( unsigned int i = 0; i < images.size(); ++i )
				{
					s[i] = images[i].at( x, y )[c];
				}
			
				double mean = 0., variance = 0., norm = 1. / s.size();
				double max = std::numeric_limits<double>::min();
				double min = std::numeric_limits<double>::max();

				double areBlack = true;
				for( unsigned int i = 0; i < s.size(); ++i )
				{
					if( s[i] != 0. )
					{
						areBlack = false;
						break;
					}
				}
				
				if( !areBlack )
				{
					bool hasAnyBlack = false;
					double count = 0.;
					for( unsigned int i = 0; i < s.size(); ++i )
					{
						if( s[i] == 0. )
						{
							hasAnyBlack = true;
							continue;
						}

						++count;

						min = ( s[i] < min ) ? s[i] : min;
						max = ( s[i] > max ) ? s[i] : max;
						mean += s[i];
					}
					mean /= count;

					if( hasAnyBlack )
					{
						for( unsigned int i = 0; i < s.size(); ++i )
						{
							if( s[i] == 0. )
							{
								continue;
							}
							double d = s[i] - mean;
							variance += d*d*( 1. / count );
						}
					
						bool flipFlop = false;
						double newMean = 0.;
						variance = 0.;
						for( unsigned int i = 0; i < s.size(); ++i )
						{
							if( s[i] == 0. )
							{
								s[i] = mean;
								newMean += mean * norm;
							}
							double d = s[i] - mean;
							variance += d*d*norm;
						}
						mean = newMean;
					}
					else
					{
						for( unsigned int i = 0; i < s.size(); ++i )
						{
							double d = s[i] - mean;
							variance += d*d*norm;
						}
					}
				}
				else
				{
					min = max = mean = variance = 0.;
				}
				
				m_min[index] = min;
				m_max[index] = max;
				m_mean[index] = mean;
				m_variance[index] = variance;
				m_deviation[index] = sqrt( variance );
				m_samples[index] = s;
				
				std::sort( s.begin(), s.end() );
				if( s.size() > 2 )
				{
					m_median[index] = s[ std::min( int( s.size()-1 ), std::max( int( ceil( s.size() / 2. ) ), 0 ) ) ];
				}
				else
				{
					m_median[index] = s.front() + ( s.back() - s.front() ) / 2.;
				}
			}
		}
	}
}

bool readPPM( const std::string &path, Image &image )
{
	FILE *f = fopen( path.c_str(), "r");
	if( f == NULL )
	{
		throw std::runtime_error( "Failed to open the image for reading." );
	}

	int width, height;
	int bytes = fscanf( f, "P3\n%d ", &width );
	if( bytes != 1 )
	{
		throw std::runtime_error( "Failed to read the width." );
	}

	bytes = fscanf( f, "%d\n", &height );
	if( bytes != 1 )
	{
		throw std::runtime_error( "Failed to read the height." );
	}

	int format = 0;
	bytes = fscanf( f, "%d", &format );
	if( bytes != 1 || format != 255 )
	{
		throw std::runtime_error( "Failed to read the format or it is not correct." );
	}

	image.resize( width, height );
	int pixel[3];
	for( int y = 0; y < image.height(); ++y )
	{
		for( int x = 0; x < image.width(); ++x )
		{
			bytes = fscanf( f, "%d %d %d ", &pixel[0], &pixel[1], &pixel[2] );
			if( bytes != 3 )
			{
				throw std::runtime_error( "Failed to read the image data." );
			}

			double *inPixel = image.writeable( x, y );
			inPixel[0] = toGamma22( pixel[0] );
			inPixel[1] = toGamma22( pixel[1] );
			inPixel[2] = toGamma22( pixel[2] );
		}
	}

	fclose(f);
	return 1;
}

bool writePPM( const std::string &path, const Image &image )
{
	FILE *f = fopen( path.c_str(), "w"); // Write image to a PPM file.
	if( !f )
	{
		throw std::runtime_error( "Failed to open the file for writing." );
	}

	int bytes = fprintf( f, "P3\n%d %d\n%d\n", image.width(), image.height(), 255 );
	if( bytes != 15 )
	{
		throw std::runtime_error( "Failed to write the image header." );
	}

	for( int y = 0; y < image.height(); ++y )
	{
		for( int x = 0; x < image.width(); ++x )
		{
			const double *pixel = image.at( x, y );
			bytes = fprintf(f,"%d %d %d ", fromGamma22( pixel[0] ), fromGamma22( pixel[1] ), fromGamma22( pixel[2] ) );
		}
	}

	fclose(f);
	return 1;
}

bool writeBMP( const std::string &path, const Image &image )
{
	std::ofstream bmp( path.c_str(), std::ios::binary );
	BmpHeader header;
	bmp.write("BM", 2);
	header.mFileSize   = ( unsigned int )( sizeof(BmpHeader) + 2 ) + image.width() * image.height() * 3;
	header.mReserved01 = 0;
	header.mDataOffset = ( unsigned int )( sizeof(BmpHeader) + 2 );
	header.mHeaderSize = 40;
	header.mWidth      = image.width();
	header.mHeight     = image.height();
	header.mColorPlates     = 1;
	header.mBitsPerPixel    = 24;
	header.mCompression     = 0;
	header.mImageSize       = image.width() * image.height() * 3;
	header.mHorizRes        = 2953;
	header.mVertRes         = 2953;
	header.mPaletteColors   = 0;
	header.mImportantColors = 0;

	bmp.write((char*)&header, sizeof(header));

	const float invGamma = 1.f / 2.2f;
	for( int y = image.height()-1; y >= 0; --y )
	{ 
		for( int x = 0; x < image.width(); ++x )
		{
			// bmp is stored from bottom up
			const double *pixel = image.at( x, y );
			typedef unsigned char byte;
			float gammaBgr[3];
			gammaBgr[0] = pow( pixel[2], invGamma ) * 255.f;
			gammaBgr[1] = pow( pixel[1], invGamma ) * 255.f;
			gammaBgr[2] = pow( pixel[0], invGamma ) * 255.f;

			byte bgrB[3];
			bgrB[0] = byte( std::min( 255.f, std::max( 0.f, gammaBgr[0] ) ) );
			bgrB[1] = byte( std::min( 255.f, std::max( 0.f, gammaBgr[1] ) ) );
			bgrB[2] = byte( std::min( 255.f, std::max( 0.f, gammaBgr[2] ) ) );

			bmp.write( (char*)&bgrB, sizeof( bgrB ) );
		}
	}
	return true;
}

