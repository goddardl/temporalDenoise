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

// C Headers
/// \todo Remove these C headers and replace with C++.
#include <math.h>
#include <stdio.h>

// C++ headers.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm> // For atoi(), atof()
#include <stdexcept>

#include "Options.h"
#include "Image.h"

/// Returns the Gaussian weight for a point 'x' in a normal distribution
/// centered at the mean with the given deviation.
double gaussian( double x, double mean, double deviation, bool normalize = true )
{
	double c = deviation;
	double a = c * sqrt( 2 * M_PI );
	double b = x - mean;
	double v = a * exp( -( ( b * b ) / ( 2*c*c ) ) );
	return normalize ? v / a : v;
}

/// A step function which has a falloff that starts when the value 'x'
/// gets within 10% of a limit. The returned value will never reach 0
/// if it is within range. Values of 'x' that are out of the range 
/// are set to 0.
double softStep( double x, double min, double max )
{
	if( x < min || x > max )
	{
		return 0;
	}

	double v = ( max - min ) * .1;
	double lower = min + v;
	double upper = max - v;
	if( x < min + v )
	{
		x = ( x - min ) / ( lower - min );
	}
	else if( x > max - v )
	{
		x = 1. - ( x - upper ) / ( max - upper );
	}
	else
	{
		return 1.;
	}

	// We ensure that the weight returns a contribution of at least .0025;
	x = .05 + ( x * .95 );

	return x * x;
}

int main( int argc, char* argv[] )
{
	//===================================================================
	// Get the command-line options.
	//===================================================================
	Options opt;
	if( !options( argc, argv , opt ) )
	{
		return 1;
	}

	// Output the options.	
	std::cerr << "Using demo sequence " << opt.sequenceNumber << "." << std::endl;
	std::cerr << "Output path is: \"" << opt.outputPath << "\"." << std::endl;
	std::cerr << "Number Of Images: " << opt.nImages << std::endl;
	std::cerr << "Start frame: " << opt.startFrame << std::endl;

	std::cerr << "Blur mode: " << ( opt.blurMode == Options::kGentle ? "Gentle" : "Aggressive") << std::endl;
	std::cerr << "Blur strength: " << opt.blurStrength << std::endl;
	std::cerr << "Contribution strength: " << opt.contributionStrength << std::endl;
	std::cerr << "Kernel width: " << opt.kernelWidth << std::endl;

	// Load the images.
	std::vector< Image > images( opt.nImages );
	int frame = opt.startFrame;
	for( unsigned int i = 0; i < images.size(); ++i )
	{
		std::stringstream s;
		s << "images/image" << opt.sequenceNumber << "." << frame << ".ppm";

		if( !readPPM( s.str(), images[i] ) )
		{
			std::cerr << "Failed to open image " << s.str() << std::endl;
			return 1;
		}
		
		if( ++frame >= 11 )
		{
			frame = 0;
		}
	}

	//===================================================================
	// The algorithm.
	//===================================================================

	SampleSet set( images );
	const int width = set.width(), height = set.height();
	Image result( width, height );

				
	int kernelRadius = opt.kernelWidth > 1 ? ( opt.kernelWidth - 1 ) / 2 : 0;
	std::vector< double > srcSamples;
	for( int y = 0; y < height; ++y )
	{
		for( int x = 0; x < width; ++x )
		{
			fprintf(stderr,"\rFiltering %5.2f%% complete.", 100. * y / ( height-1 ) );
		
			// Loop over each channel.	
			for( unsigned int c = 0; c < 3; ++c )
			{
				double destMean = set.mean( x, y, c );
				double destDeviation = set.deviation( x, y, c );
				double destVariation = set.variance( x, y, c );
				double destRange = set.max( x, y, c ) - set.min( x, y, c ); 

				// Loop over the neighbouring pixels.
				double weightedSum = 0.;
				double v = 0.;
				for( int ky = -kernelRadius; ky <= kernelRadius; ++ky )
				{
					for( int kx = -kernelRadius; kx <= kernelRadius; ++kx )
					{
						// Don't include the pixel being sampled in our calculations as we are
						// summing the deviations from it and doing so will bias our results.
						if( ky == 0 && kx == 0 )
						{
							continue;
						}

						// Gather information on the source pixel's samples.
						srcSamples = set.samples( x + kx, y + ky, c );
						double srcMin = set.min( x + kx, y + ky, c );
						double srcMax = set.max( x + kx, y + ky, c );
						double srcMean = set.mean( x + kx, y + ky, c );
						double srcDeviation = set.deviation( x + kx, y + ky, c );
						double srcVariation = set.variance( x + kx, y + ky, c );
						double srcRange = set.max( x + kx, y + ky, c ) - set.min( x + kx, y + ky, c ); 
						
						if( srcVariation == 0 && srcSamples[0] == 0. ) continue;
							
						// A gaussian falloff that weights contributing samples which are closer to the pixel being filtered higher.
						/// \todo Intuitive falloff parameters need to be added to the distance weight or at least a suitable curve found.
						double distanceWeight = gaussian( sqrt( kx*kx + ky*ky ) / sqrt( kernelRadius*kernelRadius + kernelRadius*kernelRadius ), 0., .7, false );
							
						// Similarity weight.
						// This weight defines a measure of how similar the set of contributing samples is to the pixel being filtered.
						// By itself it will produce a smart blur of sorts which is then attenuated by the variance of the source samples in the process of weighted offsets.
						// Changing this value will effect how aggressive the filtering is.
						double similarity;
						if( opt.blurMode == Options::kAggressive )
						{
							similarity = ( srcMean - destMean ) * ( srcRange - destRange );
						}
						else
						{
							similarity = ( srcMean - destMean );
						}
						similarity *= similarity;

						// Temporal weight.
						// Weight the contribution using a function in the range of 0-1 which weights the importance of the
						// contributing sample according to how close it is in time to the current time.
						double time = 1.; // \todo: implement this! Example functions are Median, Gaussian, etc.

						// Loop over each of the neighbouring samples.
						for( unsigned int i = 0; i < srcSamples.size(); ++i )
						{
							// The contribution weight extends the range of allowed samples that can influence the pixel being filtered.
							// It is simply a scaler that increases the width of the bell curve that the samples are weighted against.
							double contribution = gaussian( srcSamples[i], destMean, destDeviation * ( 1 + opt.contributionStrength ) ) * gaussian( srcSamples[i], srcMean, srcDeviation );
							contribution = contribution * ( 1. - opt.blurStrength ) + opt.blurStrength;

							// This weight is a step function with a strong falloff close to the limits. However, it will never reach 0 so that the sample is not excluded.
							// By using this weight the dependency on the limiting samples is much less which reduces the effect of sparkling artefacts.
							double limitWeight = srcSamples.size() <= 2 ? 1. : softStep( srcSamples[i], srcMin, srcMax );
						
							// Combine the weights together and normalize to the range of 0-1.	
							double weight = pow( M_E, -( similarity / ( contribution * srcVariation * time * distanceWeight * limitWeight ) ) );
							weight = ( isnan( weight ) || isinf( weight ) ) ? 0. : weight;
						
							// Sum the offset.	
							v += ( srcSamples[i] - destMean ) * weight;

							// Sum the weight.
							weightedSum += weight;
						}
					}
				}

				if( weightedSum == 0. || destVariation <= 0. )
				{
					result.writeable( x, y )[c] = destMean;
				}
				else
				{
					result.writeable( x, y )[c] = destMean + ( v / weightedSum );
				}
			}
		}
	}

	bool success = false;
	if( opt.extension == "bmp" )
	{
		success = writeBMP( opt.outputPath.c_str(), result );
	}
	else
	{
		success = writePPM( opt.outputPath.c_str(), result );
	}
	
	if( !success )
	{
		std::cerr << "Failed to write image." << std::endl;
		return 1;
	}

	return 0;
}

