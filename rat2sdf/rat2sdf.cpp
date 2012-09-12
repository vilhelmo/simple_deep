/*
 * deep_test.cpp
 *
 *  Created on: Aug 1, 2012
 *      Author: vilhelm
 */
/*
 * Copyright (c) 2012
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <iostream>
#include <IMG/IMG_DeepShadow.h>
#include <UT/UT_Exit.h>
#include <UT/UT_Options.h>
#include <UT/UT_WorkBuffer.h>

#include <deep/deep.h>
#include <deep/deepimage.h>
#include <deep/deepio.h>

namespace deep {

static void usage(const char *program) {
	cerr << "Usage: " << program << " dsmfile sdffile" << endl;
	cerr << "Converts a rat/dsm file to a simple deep file (.sdf)" << endl;
	UT_Exit::exit(UT_Exit::EXIT_GENERIC_ERROR);
}

template<typename FTYPE>
static void printTuple(const FTYPE *pixel, int vsize) {
	int i;
	printf("(%g", pixel[0]);
	for (i = 1; i < vsize; i++)
		printf(",%g", pixel[i]);
	printf(")");
}

static void printPixel(IMG_DeepShadow &fp, int x, int y) {
	int i, d, depth;
	const IMG_DeepShadowChannel *chp;
	IMG_DeepPixelReader pixel(fp);

	// Open the pixel
	if (!pixel.open(x, y)) {
		printf("\tUnable to open pixel [%d,%d]!\n", x, y);
		return;
	}

	// Get the number of z-records for the pixel
	depth = pixel.getDepth();
	printf("Pixel[%d,%d][%d]\n", x, y, depth);

	// Iterate over all channels in the DCM
	for (i = 0; i < fp.getChannelCount(); i++) {
		chp = fp.getChannel(i);
		printf(" %5s = [", chp->getName());
		if (depth) {
			// Print first depth record
			printTuple<float>(pixel.getData(*chp, 0), chp->getTupleSize());
			// And the remaining depth records
			for (d = 1; d < depth; d++) {
				printf(", ");
				printTuple<float>(pixel.getData(*chp, d), chp->getTupleSize());
			}
		}
		printf("]\n");
	}
}

static void dumpOptions(IMG_DeepShadow &fp) {
	const UT_Options * opt;
	UT_WorkBuffer wbuf;
	if (opt = fp.getTBFOptions()) {
		wbuf.strcpy("DSM Options: ");
		opt->appendPyDictionary(wbuf);
		printf("%s\n", wbuf.buffer());
	}
}

}

using namespace deep;

int main(int argc, char *argv[]) {
    IMG_DeepShadow fp;
    int xres, yres;

    if (argc < 3 || !fp.open(argv[1])) {
    	usage(argv[0]);
    }

    char * sdfFileName = argv[2];
    std::vector<std::string> channelNames;

    // Read the texture options in the file
    dumpOptions(fp);

    // Query the resolution
    fp.resolution(xres, yres);
    printf("%s[%d,%d] (%d channels)\n", argv[1], xres, yres, fp.getChannelCount());

    for (int i = 0; i < fp.getChannelCount(); i++) {
    	const IMG_DeepShadowChannel * chp = fp.getChannel(i);
    	printf("%s ", chp->getName());
    	if (strcmp(chp->getName(), "C") == 0) {
    		channelNames.push_back("R");
    		channelNames.push_back("G");
    		channelNames.push_back("B");
    		channelNames.push_back("A");
    	}
    }
    channelNames.push_back("Z");
    channelNames.push_back("ZBack");
    std::cout << std::endl;

    DeepImage * deepImage = new DeepImage(xres, yres, channelNames);
    std::cout << "Pre load" << std::endl;
    printDeepImageStats(*deepImage);

    IMG_DeepPixelReader pixel(fp);

    const IMG_DeepShadowChannel * pzp = nullptr;
    const IMG_DeepShadowChannel * ofp = nullptr;
    const IMG_DeepShadowChannel * cp = nullptr;

    for (int i = 0; i < fp.getChannelCount(); i++) {
		if (strcmp(fp.getChannel(i)->getName(), "Pz") == 0)
			pzp = fp.getChannel(i);
		else if (strcmp(fp.getChannel(i)->getName(), "Of") == 0) {
			ofp = fp.getChannel(i);
		} else if (strcmp(fp.getChannel(i)->getName(), "C") == 0) {
			cp = fp.getChannel(i);
		}
	}

//    const IMG_DeepShadowChannel * chp;
    const UT_Options * opt = fp.getTBFOptions();
    std::string interp = opt->getOptionS("texture:depth_interp");
    bool linearInterp = (interp.compare("continuous") == 0);
    int numChannels = channelNames.size();

    for (int y = 0; y < yres; ++y) {
        for (int x = 0; x < xres; ++x) {
        	if (!pixel.open(x, y)) {
				printf("\tUnable to open pixel [%d,%d]!\n", x, y);
				return 0;
			}
        	pixel.uncomposite(*pzp, *ofp, true);
        	// Get the number of z-records for the pixel
			int numSamples = pixel.getDepth();
			if (numSamples > 0) {
				for (int d = 0; d < numSamples; ++d) {
					std::vector<DeepDataType> values(numChannels);
					// Get color:
					const float * c = pixel.getData(*cp, d);
					const float * pz = pixel.getData(*pzp, d);
					const float * pzBack = pz;
					if (linearInterp) {
						if (d + 1 < numSamples) {
							pzBack = pixel.getData(*pzp, d + 1);
						} else {
							continue;
						}
					}
					for (int i = 0; i < cp->getTupleSize(); ++i) {
						values[i] = c[i];
						if (i < 3 && c[3] > 0.0) {
							// Unpremult.
							values[i] /= c[3];
						}
					}
					values[4] = pz[0];
					values[5] = pzBack[0];
//					for (int i = 0; i < fp.getChannelCount(); i++) {
//						chp = fp.getChannel(i);
//						int offset = -1;
//						if (strcmp(chp->getName(), "C") == 0) {
//							offset = 0;
//						} else if (strcmp(chp->getName(), "Pz") == 0) {
//							offset = 4;
//						}
//						if (offset >= 0) {
//							const float * p = pixel.getData(*chp, d);
//							for (int t = 0; t < chp->getTupleSize(); ++t) {
//								values[offset+t] = p[t];
//							}
//						}
//					}
//					values[5] = values[4];
//					values[0] /= values[3]; values[1] /= values[3]; values[2] /= values[3];
					deepImage->addSample(yres-y-1, x, values);
				}
			}
        }
    }

    std::cout << "Post load" << std::endl;
    printDeepImageStats(*deepImage);
    DeepImageWriter writer(sdfFileName, *deepImage);
    if (writer.open()) {
    	writer.write();
    	writer.close();
    }
    delete deepImage;

    // Print the raw pixel data
//	printPixel(fp, 0, 0);
//	printPixel(fp, xres >> 1, 0);
//	printPixel(fp, xres - 1, 0);
	printPixel(fp, 0, yres >> 1);
	printPixel(fp, xres >> 1, yres >> 1);
//	printPixel(fp, xres - 1, yres >> 1);
//	printPixel(fp, 0, yres - 1);
	printPixel(fp, xres >> 1, yres - 1);
//	printPixel(fp, xres - 1, yres - 1);

    return 0;
}

