Temporal Denoise
================

The Temporal Denoise project is supplementary material for the Siggraph 2014 talk: "Silencing the noise on Elysium".

It illustrates the spatial filtering portion of a temporal denoise algorithm which was developed to filter a sequence
of ray-traced images to remove noise. The code is the second part of a two part algorithm. For more information on the
complete algorithm, please read the short paper:

www.lukegoddard.info/siggraph2014/silencingTheNoise.pdf

Each input image - found in the "images" folder - represents a single frame from an animated sequence
which would have been acquired by the first part of the algorithm. Each pixel in each image represents
the value of a point in the scene across time. The algorithm uses these samples to remove noise by allowing samples in
the local neighbourhood to contribute to the filtered value. The weight of the contributed value is calculated using information
on the contributing pixel's sample range and variance. Sample sets with lower variance and similar resulting colour values
are weighted higher.

The project can be simply compiled using the following command line:
g++ -o denoise -I include/ src/Image.cpp src/Main.cpp src/Options.cpp

