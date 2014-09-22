/*
 *  Copyright (c) 2008, 2013 Kelly Schrock, John Hammen
 *
 *  This file is part of SDDM.
 *
 *  SDDM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SDDM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SDDM.  If not, see <http://www.gnu.org/licenses/>.
*/

// model.cpp

#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <stdio.h>
#include <string.h>

#include <sndfile.h>
#include <samplerate.h>

#include <jack/jack.h>

using namespace std;

#include "model.h"

typedef std::vector<float> FloatList;

static FloatList sample_rate_convert(SNDFILE *infile, int converter, double src_ratio, int channels);

///--- Sample
Sample::Sample(unsigned frames, const string& filename)
: filename(filename)
, dataL(NULL)
, dataR(NULL)
, frames(frames)
, sampleRate(44100)
{}

Sample::~Sample()
{
	delete[] dataL;
	delete[] dataR;
}

Sample* Sample::load(const string& filename, int maxSamples)
{
	string ext = filename.substr(filename.length()-3, filename.length());

	if(ext == "wav" || ext == "WAV")
		return loadWave(filename, maxSamples);
	else
	{
		cerr << filename << " is not a wave file. That's all I support for now. Sorry." << endl;
		return NULL;
	}
}

Sample* Sample::loadWave(const string& filename, int maxSamples)
{
	// Make sure the file exists
	std::ifstream verify(filename.c_str(), std::ios::in | std::ios::binary);
	if(verify == NULL)
	{
		cerr << "Sample::loadWave: " << filename << " not found." << endl;
		return NULL;
	}

	SF_INFO info;
	SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &info);
	if(!file)
	{
		cerr << "Sample::loadWave: error loading " << filename << endl;
		return 0;
	}

	float *buffer = new float[info.frames * info.channels];
	memset(buffer, 0, sizeof(float) * (info.frames * info.channels));
	sf_read_float(file, buffer, info.frames * info.channels);
	sf_close(file);

	int size = info.frames;
	
	if(maxSamples != -1)
		size = min((int)info.frames, maxSamples);
	
	
#ifdef _SAMPLE_RATE_CHANGE		
	const int JACK_RATE = 48000;
	
	if(info.samplerate != JACK_RATE)
	{
		FloatList sampleData;
		// Copy data from the file.
		for(int i = 0; i < size; ++i)
			sampleData.push_back(buffer[i]);

		cout << "Re-sample " << filename << " to " << JACK_RATE << " sample rate" << endl;
		
		double ratio = (1.0 * JACK_RATE) / info.samplerate;
		int converter = SRC_SINC_BEST_QUALITY;
		
		cout << "Convert by a ratio of " << ratio << " using converter " << converter << endl;
		
		if(!src_is_valid_ratio(ratio))
		{
			cerr << "Error: The sample rate change is out of valid range." << endl;
			return 0;
		}
		
		SNDFILE* in = sf_open(filename.c_str(), SFM_READ, &info);
		
		sampleData = sample_rate_convert(in, converter, ratio, info.channels);
		
		cout << "post-conversion length: " << sampleData.size() << endl;
		cout << "post-conversion buffer: " << sampleData.size() * info.channels << endl;
		
		sf_close(in);
		info.samplerate = JACK_RATE;
	}
#endif // _SAMPLE_RATE_CHANGE	

	float *dataR = new float[size];
	float *dataL = new float[size];

	switch(info.channels)
	{
		case MONO:
		{
			for(int i = 0; i < size; ++i)
			{
				dataL[i] = buffer[i];
				dataR[i] = buffer[i];
			}
			break;
		}

		case STEREO:
		{
			for(int i = 0; i < size; ++i)
			{
				dataL[i] = buffer[i*2];
				dataR[i] = buffer[i*2+1];
			}
			break;
		}
	}

	delete[] buffer;

	Sample *sample = new Sample(size, filename);
	sample->dataL = dataL;
	sample->dataR = dataR;
	sample->sampleRate = info.samplerate;
	
	return sample;
}

ostream& operator << (ostream& out, Sample& sample)
{
	out << "Sample:"
			<< " filename=" << sample.getFilename()
			<< " length=" << sample.getLength()
			<< " sampleRate=" << sample.getRate()
			<< " frames=" << sample.frames
			;
	return out;
}

#define	BUFFER_LEN	(1<<16)

static FloatList sample_rate_convert(SNDFILE *infile, int converter, double src_ratio, int channels)
{	
	FloatList out;
	
	static float input[BUFFER_LEN] ;
	static float output[BUFFER_LEN] ;

	SRC_STATE	*src_state ;
	SRC_DATA	src_data ;
	int			error ;

	sf_seek(infile, 0, SEEK_SET) ;

	/* Initialize the sample rate converter. */
	if ((src_state = src_new (converter, channels, &error)) == NULL)
	{	
		printf("\n\nError : src_new() failed : %s.\n\n", src_strerror (error)) ;
		exit(1);
	};

	src_data.end_of_input = 0 ; /* Set this later. */

	/* Start with zero to force load in while loop. */
	src_data.input_frames = 0 ;
	src_data.data_in = input ;

	src_data.src_ratio = src_ratio ;

	src_data.data_out = output ;
	src_data.output_frames = BUFFER_LEN /channels ;

	while(true)
	{
		/* If the input buffer is empty, refill it. */
		if (src_data.input_frames == 0)
		{	
			src_data.input_frames = sf_readf_float (infile, input, BUFFER_LEN / channels) ;
			src_data.data_in = input;

			/* The last read will not be a full buffer, so snd_of_input. */
			if (src_data.input_frames < BUFFER_LEN / channels)
				src_data.end_of_input = SF_TRUE;
		}

		if ((error = src_process (src_state, &src_data)))
		{	
			printf ("\nError : %s\n", src_strerror (error)) ;
			exit (1) ;
		};

		/* Terminate if done. */
		if(src_data.end_of_input && src_data.output_frames_gen == 0)
			break;
			
		/* Write output. */
		for(int i = 0; i < src_data.output_frames_gen; ++i)
			out.push_back(output[i]);

		src_data.data_in += src_data.input_frames_used;
		src_data.input_frames -= src_data.input_frames_used;
	};

	src_state = src_delete(src_state) ;
	return out;
} /* sample_rate_convert */
