// alsamidi.cpp
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

#include <map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

#include "midi.h"
#include "alsamidi.h"

#include <pthread.h>

pthread_t midiDriverThread;

bool isMidiDriverRunning = false;

snd_seq_t *seq_handle = NULL;
int npfd;
struct pollfd *pfd;
int portId;
int clientId;

void* alsaMidiDriver_thread(void* param)
{
	AlsaMidiDriver *pDriver = (AlsaMidiDriver*)param;

	if(seq_handle != NULL) 
	{
		cerr << "alsaMidiDriver_thread(): seq_handle != NULL" << endl;
		pthread_exit(NULL);
	}

	int err;
	if((err = snd_seq_open( &seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0 )) < 0) 
	{
		cerr << "Error opening ALSA sequencer: " + string(snd_strerror(err))  << endl;
		pthread_exit(NULL);
	}

	snd_seq_set_client_name(seq_handle, pDriver->getClientName().c_str());

	if((portId = snd_seq_create_simple_port(seq_handle,
									"midi in",
									SND_SEQ_PORT_CAP_WRITE |
									SND_SEQ_PORT_CAP_SUBS_WRITE,
									SND_SEQ_PORT_TYPE_APPLICATION
								)) < 0)
	{
		cerr << "Error creating sequencer port.";
		pthread_exit(NULL);
	}
	
	clientId = snd_seq_client_id(seq_handle);

	int m_local_addr_port = portId;
	int m_local_addr_client = clientId;

	string sPortName = pDriver->getConnectPortName();
	int m_dest_addr_port = -1;
	int m_dest_addr_client = -1;
	
	pDriver->getPortInfo(sPortName, m_dest_addr_client, m_dest_addr_port);
	cout << "MIDI port name: " << sPortName << endl;
	cout << "MIDI addr client: " << m_dest_addr_client << endl;
	cout << "MIDI addr port: " << m_dest_addr_port << endl;

	if((m_dest_addr_port != -1) && (m_dest_addr_client != -1)) 
	{
		snd_seq_port_subscribe_t *subs;
		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_addr_t sender, dest;

		sender.client = m_dest_addr_client;
		sender.port = m_dest_addr_port;
		dest.client = m_local_addr_client;
		dest.port = m_local_addr_port;

		/* set in and out ports */
		snd_seq_port_subscribe_set_sender(subs, &sender);
		snd_seq_port_subscribe_set_dest(subs, &dest);

		/* subscribe */
		int ret = snd_seq_subscribe_port(seq_handle, subs);
		if(ret < 0)
			cerr << "snd_seq_connect_from(" << m_dest_addr_client << ":" << m_dest_addr_port << " error" << endl;
	}

	cout << "Midi input port at " << clientId << ":" << portId << endl;

	npfd = snd_seq_poll_descriptors_count( seq_handle, POLLIN );
	pfd = ( struct pollfd* )alloca( npfd * sizeof( struct pollfd ) );
	snd_seq_poll_descriptors( seq_handle, pfd, npfd, POLLIN );

	while(isMidiDriverRunning) 
	{
		if(poll( pfd, npfd, 100) > 0) 
		{
			pDriver->midi_action( seq_handle );
		}
	}
	
	snd_seq_close(seq_handle);
	seq_handle = NULL;

	pthread_exit(NULL);
	return NULL;
}

AlsaMidiDriver::~AlsaMidiDriver()
{
	if (isMidiDriverRunning) 
	{
		close();
	}
}

void AlsaMidiDriver::open(string &name)
{
    clientName = name;
	// start main thread
	isMidiDriverRunning = true;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&midiDriverThread, &attr, alsaMidiDriver_thread, this);
}

void AlsaMidiDriver::close()
{
	isMidiDriverRunning = false;
	pthread_join(midiDriverThread, NULL);
}

void AlsaMidiDriver::connectPort(string &port, string &target)
{
	unsigned cl = -1;
	unsigned pt = -1;
	
	string::size_type pos = port.find_first_of(":", 0);
	if(string::npos != pos)
	{
		string first = port.substr(0, pos);
		string second = port.substr(pos+1);
		
		cout << "first=" << first << " second=" << second << endl;
		
		cl = atoi(first.c_str());
		pt = atoi(second.c_str());
	}
	
	// portId and clientId might be of some use here. They are the port id and the client id of this app.
	// Need some way of getting the client id and port id of the other client by name.
	if(cl != -1 && pt != -1)
	{
		if(0 == snd_seq_connect_from(seq_handle, portId, cl, pt))
			;
		else
			cerr << "Unable to connect to the specified MIDI client/port: " << port << endl;	
	}
}

void AlsaMidiDriver::midi_action(snd_seq_t *seq_handle)
{
	snd_seq_event_t *ev;
	do {
		if (!seq_handle) {
			break;
		}
		snd_seq_event_input(seq_handle, &ev);

		if(active) {

			MidiMessage msg;
			
			switch(ev->type) {
				case SND_SEQ_EVENT_NOTEON:
					msg.type = MidiMessage::NOTE_ON;
					msg.data1 = ev->data.note.note;
					msg.data2 = ev->data.note.velocity;
					msg.channel = ev->data.control.channel;
					break;

				case SND_SEQ_EVENT_NOTEOFF:
					msg.type = MidiMessage::NOTE_OFF;
					msg.data1 = ev->data.note.note;
					msg.data2 = ev->data.note.velocity;
					msg.channel = ev->data.control.channel;
					break;

				case SND_SEQ_EVENT_CONTROLLER:
					msg.type = MidiMessage::CONTROL_CHANGE;
					break;

				case SND_SEQ_EVENT_SYSEX:
				{
					snd_midi_event_t *seq_midi_parser;
					if( snd_midi_event_new(32, &seq_midi_parser) ) {
						cerr << "[midi_action] error creating midi event parser" << endl;
					}
					
					unsigned char midi_event_buffer[256];
					int _bytes_read = snd_midi_event_decode( seq_midi_parser, midi_event_buffer, 32, ev );

					sysexEvent(midi_event_buffer, _bytes_read);

					cout << "SND_SEQ_EVENT_SYSEX" << endl;
				}
				break;

				case SND_SEQ_EVENT_QFRAME:
	//				cout << "quarter frame" << endl;
//					if ( useMidiTransport ) {
//						playEvent();
//					}
					break;

				case SND_SEQ_EVENT_CLOCK:
	//				cout << "Midi CLock" << endl;
					break;

				case SND_SEQ_EVENT_SONGPOS:
					msg.type = MidiMessage::SONG_POS;
					break;

				case SND_SEQ_EVENT_START:
					msg.type = MidiMessage::START;
					break;

				case SND_SEQ_EVENT_CONTINUE:
					msg.type = MidiMessage::CONTINUE;
					break;

				case SND_SEQ_EVENT_STOP:
					msg.type = MidiMessage::STOP;
					break;

				case SND_SEQ_EVENT_PITCHBEND:
					break;

				case SND_SEQ_EVENT_PGMCHANGE:
					break;

				case SND_SEQ_EVENT_CLIENT_EXIT:
					cout << "SND_SEQ_EVENT_CLIENT_EXIT" << endl;
					break;

				case SND_SEQ_EVENT_PORT_SUBSCRIBED:
					cout << "SND_SEQ_EVENT_PORT_SUBSCRIBED" << endl;
					break;

				case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
					cout << "SND_SEQ_EVENT_PORT_UNSUBSCRIBED" << endl;
					break;

				case SND_SEQ_EVENT_SENSING:
					//cout << "SND_SEQ_EVENT_SENSING" << endl;
					break;

				default:
					cout << "AlsaMidiDriver: Unknown MIDI Event. type=" << ((int)ev->type) << endl;
			}
			
			handleMidiMessage( msg );
		}
		
		snd_seq_free_event( ev );
	} while(snd_seq_event_input_pending(seq_handle, 0) > 0);
}

/*
void AlsaMidiDriver::controllerEvent( snd_seq_event_t* ev )
{
	int channel = ev->data.control.channel;
	unsigned param = ev->data.control.param;
	unsigned value = ev->data.control.value;
	unsigned controllerParam = ev->data.control.param;

	switch ( controllerParam ) {
		case 1:	// ModWheel
			infoLog( "[controllerEvent] ModWheel control event" );
			break;

		case 7:	// Volume
			infoLog( "[controllerEvent] Volume control event" );
			break;

		case 10:	// Pan
			infoLog( "[controllerEvent] Pan control event" );
			break;

		case 11:	// Expression
			infoLog( "[controllerEvent] Expression control event" );
			break;

		case 64:	// Sustain
			infoLog( "[controllerEvent] Sustain control event" );
			break;

		case 66:	// Sostenuto
			infoLog( "[controllerEvent] Sostenuto control event" );
			break;

		case 91:	// reverb
			infoLog( "[controllerEvent] Reverb control event" );
			break;

		case 93:	// chorus
			infoLog( "[controllerEvent] Chorus control event" );
			break;

		default:
			warningLog( "[controllerEvent] Unhandled Control event" );
			warningLog( "[controllerEvent] controller param = " + toString(param) );
			warningLog( "[controllerEvent] controller channel = " + toString(channel) );
			warningLog( "[controllerEvent] controller value = " + toString(value) );
	}
}
*/

void AlsaMidiDriver::sysexEvent(unsigned char *buf, int nBytes)
{
	cout << "sysexEvent" << endl;
//	infoLog( "[sysexEvent]" );

	/*
		General MMC message
		0	1	2	3	4	5
		240	127	id	6	cmd	247

		cmd:
		1	stop
		2	play
		3	Deferred play
		4	Fast Forward
		5	Rewind
		6	Record strobe (punch in)
		7	Record exit (punch out)
		9	Pause


		Goto MMC message
		0	1	2	3	4	5	6	7	8	9	10	11	12
		240	127	id	6	68	6	1	hr	mn	sc	fr	ff	247
	*/


	if ( nBytes == 6 ) {
		if ( ( buf[0] == 240 ) && ( buf[1] == 127 ) && ( buf[2] == 127 ) && ( buf[3] == 6 ) ) {
			switch (buf[4] ) {
				case 1:	// STOP
// 					infoLog( "[sysexEvent] MMC STOP" );
//					stopEvent();
					break;

				case 2:	// PLAY
					cerr << "[sysexEvent] MMC PLAY not implemented yet." << endl;
					break;

				case 3:	//DEFERRED PLAY
					cerr << "[sysexEvent] MMC DEFERRED PLAY not implemented yet." << endl;
					break;

				case 4:	// FAST FWD
					cerr << "[sysexEvent] MMC FAST FWD not implemented yet." << endl;
					break;

				case 5:	// REWIND
					cerr << "[sysexEvent] MMC REWIND not implemented yet." << endl;
					break;

				case 6:	// RECORD STROBE (PUNCH IN)
					cerr << "[sysexEvent] MMC PUNCH IN not implemented yet." << endl;
					break;

				case 7:	// RECORD EXIT (PUNCH OUT)
					cerr << "[sysexEvent] MMC PUNCH OUT not implemented yet." << endl;
					break;

				case 9:	//PAUSE
					cerr << "[sysexEvent] MMC PAUSE not implemented yet." << endl;
					break;

				default:
					cerr << "[sysexEvent] Unknown MMC Command" << endl;
//					midiDump( buf, nBytes );
			}
		}
	}
	else if ( nBytes == 13 ) {
		
		cerr << "[sysexEvent] MMC GOTO Message not implemented yet" << endl;
//		midiDump( buf, nBytes );
		//int id = buf[2];
		int hr = buf[7];
		int mn = buf[8];
		int sc = buf[9];
		int fr = buf[10];
		int ff = buf[11];
		char tmp[200];
		sprintf( tmp, "[sysexEvent] GOTO %d:%d:%d:%d:%d", hr, mn, sc, fr, ff );
		cout << tmp << endl;
// 		infoLog( tmp );

	}
	else {
		cerr << "[sysexEvent] Unknown SysEx message" << endl;
//		midiDump( buf, nBytes );
	}
	
// 	EventQueue::getInstance()->pushEvent( EVENT_MIDI_ACTIVITY, -1 );
}

PortList AlsaMidiDriver::getOutputPortList()
{
	vector<string> outputList;

	if ( seq_handle == NULL ) {
		return outputList;
	}

	snd_seq_client_info_t *cinfo;	// client info
	snd_seq_port_info_t *pinfo;	// port info

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_client_info_set_client(cinfo, -1);

	/* while the next client one the sequencer is avaiable */
	while ( snd_seq_query_next_client( seq_handle, cinfo ) >= 0 ) {
		// get client from cinfo
		int client = snd_seq_client_info_get_client( cinfo );

		// fill pinfo
		snd_seq_port_info_alloca( &pinfo );
		snd_seq_port_info_set_client( pinfo, client );
		snd_seq_port_info_set_port( pinfo, -1 );

		// while the next port is available
		while ( snd_seq_query_next_port( seq_handle, pinfo ) >= 0 ) {

			/* get its capability */
			int cap =  snd_seq_port_info_get_capability(pinfo);

			if ( snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo) && snd_seq_port_info_get_client(pinfo) != 0 ) {
				// output ports
				if  (
					(cap & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0 &&
					snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo)
				) {
					outputList.push_back( snd_seq_port_info_get_name(pinfo) );
					//info.m_nClient = snd_seq_port_info_get_client(pinfo);
					//info.m_nPort = snd_seq_port_info_get_port(pinfo);
				}
			}
		}
	}

	return outputList;
}

void AlsaMidiDriver::getPortInfo(const std::string& sPortName, int& nClient, int& nPort)
{
	if(!seq_handle)
		return;

	if(sPortName == "None")
	{
		nClient = -1;
		nPort = -1;
		return;
	}

	snd_seq_client_info_t *cinfo;	// client info
	snd_seq_port_info_t *pinfo;	// port info

	snd_seq_client_info_alloca( &cinfo );
	snd_seq_client_info_set_client( cinfo, -1 );

	/* while the next client one the sequencer is avaiable */
	while(snd_seq_query_next_client( seq_handle, cinfo ) >= 0 ) {
		// get client from cinfo
		int client = snd_seq_client_info_get_client( cinfo );

		// fill pinfo
		snd_seq_port_info_alloca( &pinfo );
		snd_seq_port_info_set_client( pinfo, client );
		snd_seq_port_info_set_port( pinfo, -1 );

		// while the next port is avail
		while(snd_seq_query_next_port( seq_handle, pinfo ) >= 0 ) 
		{
			int cap =  snd_seq_port_info_get_capability(pinfo);
			if(snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo) 
				&& snd_seq_port_info_get_client(pinfo) != 0) 
			{
				// output ports
				if((cap & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0 
				&&	snd_seq_client_id( seq_handle ) != snd_seq_port_info_get_client(pinfo))
				{
					string sName = snd_seq_port_info_get_name(pinfo);
					if ( sName == sPortName ) {
						nClient = snd_seq_port_info_get_client(pinfo);
						nPort = snd_seq_port_info_get_port(pinfo);
						return;
					}
				}
			}
		}
	}
}
