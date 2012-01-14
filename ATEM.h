/*
Copyright 2012 Kasper Skårhøj, SKAARHOJ, kasperskaarhoj@gmail.com

This file is part of the ATEM library for Arduino

The ATEM library is free software: you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by the 
Free Software Foundation, either version 3 of the License, or (at your 
option) any later version.

The ATEM library is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with the ATEM library. If not, see http://www.gnu.org/licenses/.

*/


/**
  Version 1 beta
**/


#ifndef ATEM_h
#define ATEM_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif


#include <EthernetUdp.h>


class ATEM
{
  private:
	EthernetUDP _Udp;	// Udp Object for communication, see constructor.
	uint16_t _localPort; // local port to send from
	IPAddress _switcherIP;	// IP address of the switcher
	boolean _serialOutput;

	uint8_t _sessionID;		// Used internally for storing packet size during communication
	char _packetBuffer[96];   // Buffer for storing segments of the packets from ATEM. Has the size of the largest known "segment" of a packet the ATEM sends.

	uint16_t _localPacketIdCounter;  	// This is our counter for the command packages we might like to send to ATEM. 
	boolean _hasInitialized;  			// If true, the initial reception of the ATEM memory has passed and we can begin to respond during the runLoop()
	uint8_t _answer[24]; 				// Little buffer for creating answers back to the ATEM

		// Selected ATEM Status values:
	uint8_t _ATEM_PrgI;		// Program input channel
	uint8_t _ATEM_PrvI;		// Preview input channel

  public:
    ATEM(IPAddress ip, uint16_t localPort);
    void connect();
    void runLoop();
  	void serialOutput(boolean serialOutput);


  private:
	void _sendAnswerPacket(uint16_t remotePacketID);
	void _parsePacket(uint16_t packetLength);
	void _sendCommandPacket(char cmd[4], uint8_t commandBytes[16], uint8_t cmdBytes);
	
  public:
	void setATEM_PrgI(uint8_t inputNumber);
	uint8_t getATEM_PrgI();
	void setATEM_PrvI(uint8_t inputNumber);
	uint8_t getATEM_PrvI();
	void send_cut();
};

#endif

