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
#include <iostream>
#include <algorithm> // For atoi(), atof()

#include "Options.h"

/// Prints the help message when using the -h option.
static void helpMessage( std::string name )
{
    std::cerr << "Usage: " << name << " [ -h | -n <numberOfImages> | -b <blur> | -k <opt.kernelWidth> | -c <contribution> | -i <imageSequence> | -o <output> ]" << std::endl
              << "Options:" << std::endl
              << "\t-h, --help\t\tShow this help message." << std::endl
              << "\t-o, --output X\t\tSpecifies the output path. The supported file types are PPM and BMP." << std::endl
              << "\t-i, --image X\t\tChange the preset sequence of images to filter. The argument must be an integer in the range of 0-4." << std::endl
              << "\t-n, --numberOfImages X\tSpecify the number of images to used. 10 is the maximum." << std::endl
              << "\t-b, --blur X\t\tSpecify the amount of smart blur to apply. The range is 0-1 and the default is 0.005." << std::endl
			  << "\t\t\t\tBe aware that when the smart blur is fully on, the \"contribution\" weight will have no effect." << std::endl
			  << "\t-bm, --blurMode X\tSpecifies the type of smart blur used by the algorithm. 0: Agressive, 1: Regular." << std::endl
			  << "\t\t\t\tThe default is \"Aggressive\" as the effect can always be attenuated using the contribution parameter." << std::endl
              << "\t-k, --kernelWidth X\tSets the width of the spatial filtering kernel. Must be an odd number." << std::endl
              << "\t-c, --contribution X\tIncreases the value range of local pixels that can contribute to the filtered result." << std::endl
			  << "\t\t\t\tThe contribution weight will have no effect if the blur has a value of 1." << std::endl
              << "\t-s, --startFrame X\tAllows the start frame that will be used from the sequence to be specified. Note that the" << std::endl
			  << "\t\t\t\tsequence will loop if the number of required images extends past those which are available." << std::endl
			  << "\t\t\t\tBy increasing this value, high frequency noise that is present in the filtered image which is the result of" << std::endl
			  << "\t\t\t\tundersampling in the render is reduced." << std::endl
              << std::endl;
}

bool options( int argc, char* argv[], Options &opt )
{
	const int maxImages = 10;

	// Get any user overrides.
	for( int i = 1; i < argc; ++i )
	{
        std::string arg = argv[i];
        if( ( arg == "-h" ) || ( arg == "--help" ) )
		{
            helpMessage( argv[0] );
            return 0;
        }
		else if( ( arg == "-n" ) || ( arg == "--numberOfImages" ) )
		{
            if( i + 1 < argc )
			{
                opt.nImages = ::atoi( argv[++i] );
				if( opt.nImages < 0 )
				{
					opt.nImages = 0;
				}
				if( opt.nImages > maxImages )
				{
					std::cerr << "There are only 11 uniques images available per sequence. Be aware that multiple frames will be reused." << std::endl;
				}
            }
			else
			{
				std::cerr << "--numberOfImages option requires one argument." << std::endl;
                return 0;
            }  
        }
		else if( ( arg == "-s" ) || ( arg == "--opt.startFrame" ) )
		{
            if( i + 1 < argc )
			{
                opt.startFrame = ::atoi( argv[++i] );
				while( opt.startFrame < 0 ) opt.startFrame += maxImages;
				while( opt.startFrame > maxImages ) opt.startFrame -= maxImages;
            }
			else
			{
				std::cerr << "--opt.startFrame option requires one argument." << std::endl;
                return 0;
            }  
        }
		else if( ( arg == "-i" ) || ( arg == "--image" ) )
		{
            if( i + 1 < argc )
			{
                opt.sequenceNumber = ::atoi( argv[++i] );
				if( opt.sequenceNumber < 0 || opt.sequenceNumber > 5 )
				{
					opt.sequenceNumber = 0;
					std::cerr << "There are only 5 preset sequences. Selecting sequence 0." << std::endl;
				}
            }
			else
			{
				std::cerr << "--image option requires one argument." << std::endl;
                return 0;
            }  
        }
		else if( ( arg == "-bm" ) || ( arg == "--blurMode" ) )
		{
            if( i + 1 < argc )
			{
				int mode = ::atoi( argv[++i] );
				if( mode == 0 )
				{
					opt.blurMode = Options::kAggressive;
				}
				else if( mode == 1 )
				{
					opt.blurMode = Options::kGentle;
				}
				else
				{
					opt.blurMode = Options::kAggressive;
					std::cerr << "The blurMode option must have a value of 0 or 1. Using the default." << std::endl;
				}
            }
			else
			{
				std::cerr << "--contribution option requires one argument." << std::endl;
                return 0;
            }  
        }
		else if( ( arg == "-c" ) || ( arg == "--contribution" ) )
		{
            if( i + 1 < argc )
			{
                opt.contributionStrength = ::atof( argv[++i] );
				if( opt.contributionStrength < 0 )
				{
					opt.contributionStrength = 0;
					std::cerr << "The contribution weight cannot be less than 0. Clamping it to 0." << std::endl;
				}
            }
			else
			{
				std::cerr << "--contribution option requires one argument." << std::endl;
                return 0;
            }  
        }
		else if( ( arg == "-o" ) || ( arg == "--output" ) )
		{
            if( i + 1 < argc )
			{
				std::string path( argv[++i] );

				if( path.length() == 0 )
				{
					std::cerr << "Invalid output path specified. Using the default." << std::endl;
				}
				else
				{
					opt.outputPath = path;
				}
            }
			else
			{
				std::cerr << "--output option requires one argument." << std::endl;
                return 0;
            }  
        }
		else if( ( arg == "-b" ) || ( arg == "--blur" ) )
		{
            if( i + 1 < argc )
			{
                opt.blurStrength = ::atof( argv[++i] );
				if( opt.blurStrength < 0 || opt.blurStrength > 1 )
				{
					opt.blurStrength = std::min( 1., std::max( opt.blurStrength, 0. ) );
					std::cerr << "The blur must be in the range of 0-1. Clamping to " << opt.blurStrength << "." << std::endl;
				}
            }
			else
			{
				std::cerr << "--blur option requires one argument." << std::endl;
                return 0;
            }  
        }
		else if( ( arg == "-k" ) || ( arg == "--opt.kernelWidth" ) )
		{
            if( i + 1 < argc )
			{
                opt.kernelWidth = ::atoi( argv[++i] );
				if( opt.kernelWidth % 2 == 0 )
				{
					opt.kernelWidth += 1;
					std::cerr << "The kernel width must be an odd number. Rounding up to " << opt.kernelWidth << "." << std::endl;
				}
            }
			else
			{
				std::cerr << "--opt.kernelWidth option requires one argument." << std::endl;
                return 0;
            }  
        }
		else
		{
			std::cerr << "Unknown argument \"" << arg << "\"" << std::endl;
            helpMessage( argv[0] );
            return 0;
		}
    }

	if( argc == 1 )
	{
		std::cerr << "Please run: \"" << argv[0] << " --help\" for a complete list of the available options." << std::endl;
	}
   
	// Make sure that the output path is valid. 
    std::string extension = opt.outputPath.substr( opt.outputPath.length() - 3, 3 );
	if( extension != "bmp" && extension != "ppm" )
	{
		opt.outputPath += ".bmp";
	}

	return true;
}

