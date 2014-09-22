// audio_driver.cpp
// a source file
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

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include <pthread.h>

#include <sndfile.h>
#include <jack/jack.h>

using namespace std;

#include "model.h"
#include "midi.h"
#include "alsamidi.h"
#include "audio_driver.h"

// static stuff
const char* AudioDriver::LEFT_PORT_NAME = "left";
const char* AudioDriver::RIGHT_PORT_NAME = "right";

pthread_t jackThread;

//
// static stuff
//
typedef jack_default_audio_sample_t sample_t;
jack_client_t *JackAudioDriver::client = 0;

int JackAudioDriver::jackProcess(jack_nframes_t frames, void* arg)
{
	((JackAudioDriver*)arg)->process(frames);
	return 0;
}

int JackAudioDriver::jackBufferSize(jack_nframes_t frames, void* arg)
{
	((JackAudioDriver*)arg)->onJackBufferSize(frames);
	return 0;
}

int JackAudioDriver::jackSampleRate(jack_nframes_t frames, void* arg)
{
	((JackAudioDriver*)arg)->onJackSampleRateChange(frames);
	return 0;
}

void JackAudioDriver::jackOff(void *arg)
{
	((JackAudioDriver*)arg)->onJackShutdown();
}

//
// JackAudioDriver
//


JackAudioDriver::~JackAudioDriver()
{
  jack_port_unregister(client, left);
  jack_port_unregister(client, right);

	jack_client_close(client);
	JackAudioDriver::client = 0;
}

void JackAudioDriver::onJackSampleRateChange(jack_nframes_t frames)
{
	cout << "jack sample rate set to " << frames << " frames" << endl;
}

void JackAudioDriver::onJackBufferSize(jack_nframes_t frames)
{
	cout << "jack buffer size set to " << frames << " frames" << endl;
}

void JackAudioDriver::close()
{
	jack_client_close(client);
	JackAudioDriver::client = 0;
}

void* jack_thread(void* /*param*/)
{
	cout << "TEST Jack thread" << endl;
	
	pthread_exit(NULL);
	return NULL;
}

bool JackAudioDriver::registerPort(string & name)
{
	jack_port_t * port = jack_port_register(JackAudioDriver::client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	if(port)
	{
		portMap[name] = port;
	}
	
	return (port != 0);
}

bool JackAudioDriver::unregisterPort(string& name)
{
	jack_port_t * port = portMap[name];
	int rc = jack_port_unregister(JackAudioDriver::client, port);
	portMap.erase(name);
	return rc == 0;
}

bool JackAudioDriver::hasRegisteredPort(string &name) {
	for (JackAudioDriver::JackPortMap::iterator e = portMap.begin();
			e != portMap.end(); ++e) {
		if (((*e)).first == name) {
			return true;
		}
	}
	return false;
}

AudioDriver::ClientNameList JackAudioDriver::getClientNames()
{
	AudioDriver::ClientNameList clients;
	const char ** readports = jack_get_ports(JackAudioDriver::client, 0, 0, JackPortIsOutput | JackPortIsInput);
	int port = 0;
	while(readports && readports[port])
	{
		string portName = string(readports[port++]);
		
		int pos = portName.find(":");
		string nn = portName.substr(0, pos);
		
		bool found = false;
		for(unsigned i = 0; i < clients.size(); ++i)
		{
			if(clients[i] == nn)
			{
				found = true;
				break;
			}
		}
		
		if(!found)
			clients.push_back(nn);
	}
	
	return clients;
}

bool JackAudioDriver::connectPort(string &port, string &target)
{
	cout << port << "\t\t->\t\t" << target << "...";
	
	char *portName = new char[this->clientName.length() + port.length() + 1];
	sprintf(portName, "%s:%s", this->clientName.c_str(), port.c_str());
	
//	const char **ports = jack_get_ports(JackAudioDriver::client, NULL, NULL, JackPortIsInput);
//	cout << "ports[0]=" << ports[0] << endl;
	
	int rc = jack_connect(JackAudioDriver::client, portName, target.c_str());
	
	delete portName;
	cout << ((rc==0)? "ok":(rc==EEXIST)? "already connected": "failed") << endl;
	return true;
}

bool JackAudioDriver::connectMainStereoOut(string &leftPort, string &rightPort)
{
	const char **mainOutPorts = jack_get_ports(JackAudioDriver::client, 0,
			JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsPhysical);
	if (mainOutPorts)
	{
		string mainOutLeft = string(mainOutPorts[0]);
		string mainOutRight = string(mainOutPorts[1]);
		connectPort(leftPort, mainOutLeft);
		connectPort(rightPort, mainOutRight);
		::free(mainOutPorts);
		return true;
	}
	return false;
}


bool JackAudioDriver::open(string &name)
{
    clientName = name;
	cout << "JackAudioDriver::open(): Client name=" << clientName << endl;
	
	jack_status_t status;
	
	JackAudioDriver::client = jack_client_open(clientName.c_str(), JackUseExactName, &status, NULL);
	if(!JackAudioDriver::client)
	{
		for(int i = 1; i < 20; ++i)
		{
			char buf[100];
			sprintf(buf, "%s-0%d", clientName.c_str(), i);
			
			string client = buf;
			
			// TODO: jack_client_new() is deprecated. Fix this at some point
			JackAudioDriver::client = jack_client_open(client.c_str(), JackUseExactName, &status, NULL);
			if(JackAudioDriver::client)
			{
				clientName = client;
				cout << "Registered " << clientName << " client" << endl;
				break;
			}
		}
		
		if(!JackAudioDriver::client)
		{
			cerr << "Unable to register a Jack client!" << endl;
			return false;
		}
	}
	
	sampleRate = jack_get_sample_rate(client);

	jack_set_process_callback(client, jackProcess, this);
  jack_set_sample_rate_callback(client, jackSampleRate, this);
  jack_set_buffer_size_callback(client, jackBufferSize, this);
  jack_on_shutdown(client, jackOff, this);
  
	cout << "jack buffer size: " << jack_get_buffer_size(client) << endl;
  
  cout << "activate Jack client" << endl;
  if(jack_activate(client))
  {
    cerr << "error: could not activate jack client!" << endl;
    return false;
  }

	cout << "Register main ports:";
  left = jack_port_register(client, LEFT_PORT_NAME, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  right = jack_port_register(client, RIGHT_PORT_NAME, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  if((!left) || (!right))
  {
  	cerr << "Error registering one or both of the main ports." << endl;
  	return false;
  }
  
  portMap[LEFT_PORT_NAME] = left;
  portMap[RIGHT_PORT_NAME] = right;
  
  cout << ":ok" << endl;
  
  bool autoConnect = true; // TODO: make an option in the GUI
  if(autoConnect) {
	  string leftPort = string(LEFT_PORT_NAME);
	  string rightPort = string(RIGHT_PORT_NAME);
	  if(!connectMainStereoOut(leftPort, rightPort)) {
		  cerr << "Error connecting to main stereo outputs";
	  }
  }
	return true;
}

void JackAudioDriver::onJackShutdown()
{
	cout << "Jack shutdown" << endl;
	close();
	exit(1);
}

void JackAudioDriver::process(jack_nframes_t frames)
{
	// Make sure we're up and running
	if(left && right)
	{
		// For each listener
		for(AudioListenerList::iterator e = listeners.begin(); e != listeners.end(); ++e)
		{
			IAudioListener *listener = *e;
			
			// See what buffers they want.
			BufferRequestList requests = listener->getBufferRequests();
			if(!requests.empty())
			{
				// For each buffer request
				for(BufferRequestList::iterator f = requests.begin(); f != requests.end(); ++f)
				{
					BufferRequest* req = *f;
					// Create a response for this request (on the stack, so it cleans itself up).
					BufferResponse response(req, frames);
					
					// Find out which ports they're interested in.
					BufferRequest::PortNameList ports = req->getPortNames();
					unsigned idx = 0;
					for(BufferRequest::PortNameList::iterator g = ports.begin(); g != ports.end(); ++g)
					{
						// Get the jack port for this request/port name
						jack_port_t* jackPort = portMap[*g];
						// Allocate a buffer for it if found
						if(jackPort)
						{
							sample_t* buf = (sample_t*)jack_port_get_buffer(jackPort, frames);
							memset(buf, 0, sizeof(sample_t) * frames);
							
							response.add(*g, buf);
							
							// Make sure we only fill two buffers, since we're stereo.
							if(idx == 0)
								response.setLeft(buf);
							
							if(idx == 1)
								response.setRight(buf);
								
							if(++idx >= 2)
								break;
						}
					} // for(PortNames...)
					
					listener->play(&response);
					
				} // for(BufferRequests...)
			}
		} // for(Listeners...)
	}
}

//
// ------------------- BufferResponse
//
BufferResponse::~BufferResponse()
{
	if(request)
		delete request;
}
