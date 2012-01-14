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

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "ATEM.h"

ATEM::ATEM(IPAddress ip, uint16_t localPort){
		// Set up Udp communication object:
	EthernetUDP Udp;
	_Udp = Udp;
	
	_switcherIP = ip;	// Set switcher IP address
	_localPort = localPort;	// Set local port (just a random number I picked)
}

void ATEM::connect() {

	_localPacketIdCounter = 1;	// Init localPacketIDCounter to 1;
	_hasInitialized = false;
	_Udp.begin(_localPort);

	// Send connectString to ATEM:
	// TODO: Describe packet contents according to rev.eng. API
	byte connectHello[] = {  
		0x10, 0x14, 0x53, 0xAB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	_Udp.beginPacket(_switcherIP,  9910);
	_Udp.write(connectHello,20);
	_Udp.endPacket();   

	// Waiting for the ATEM to answer back with a packet 20 bytes long.
	// According to packet analysis with WireShark, this feedback from ATEM
	// comes within a few microseconds!
	uint16_t packetSize = 0;
	while(packetSize!=20) {
		packetSize = _Udp.parsePacket();
	}

	// Read the response packet. We will only subtract the session ID
	// According to packet analysis with WireShark, this feedback from ATEM
	// comes a few microseconds after our connect invitation above. Two packets immediately follow each other.
	// After approx. 200 milliseconds a third packet is sent from ATEM - a sort of re-sent because it gets impatient.
	// And it seems that THIS third packet is the one we actually read and respond to. In other words, I believe that 
	// the ethernet interface on Arduino actually misses the first two for some reason!
	while(!_Udp.available()){}  // Waiting.... TODO: Implement some way to exit if there is no answer!

	_Udp.read(_packetBuffer,20);
	_sessionID = _packetBuffer[15];


	// Send connectAnswerString to ATEM:
	_Udp.beginPacket(_switcherIP,  9910);
	// TODO: Describe packet contents according to rev.eng. API
	byte connectHelloAnswerString[] = {  
	  0x80, 0x0c, 0x53, 0xab, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00 };
	_Udp.write(connectHelloAnswerString,12);
	_Udp.endPacket();
}

void ATEM::runLoop() {

  // WARNING:
  // It can cause severe timing problems using "slow" functions such as Serial.print*() 
  // in the runloop, in particular during "boot" where the ATEM delivers some 10-20 kbytes of system status info which
  // must exit the RX-buffer quite fast. Therefore, using Serial.print for debugging in this 
  // critical phase will in it self affect program execution!

  // Limit of the RX buffer of the Ethernet interface is another general issue.
  // When ATEM sends the initial system status packets (10-20 kbytes), they are sent with a few microseconds in between
  // The RX buffer of the Ethernet interface on Arduino simply does not have the kapacity to take more than 2k at a time.
  // This means, that we only receive the first packet, the others seems to be discarded. Luckily most information we like to 
  // know about is in the first packet (and some in the second, the rest is probably thumbnails for the media player).
  // It may be possible to bump up this buffer to 4 or 8 k by simply re-configuring the amount of allowed sockets on the interface.
  // For some more information from a guy seemingly having a similar issue, look here:
  // http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1282170842



  // If there's data available, read a packet
  uint16_t packetSize = _Udp.parsePacket();
  if (_Udp.available() && packetSize !=0)   {  

    // Read packet header of 12 bytes:
    _Udp.read(_packetBuffer, 12);

    // Read out packet length (first word), remote packet ID number and "command":
    uint16_t packetLength = word(_packetBuffer[0] & B00000111, _packetBuffer[1]);
    uint16_t remotePacketID = word(_packetBuffer[10],_packetBuffer[11]);
    uint8_t command = _packetBuffer[0] & B11111000;
    boolean command_ACK = command & B00001000 ? true : false;	// If true, ATEM expects an acknowledgement answer back!
		// The five bits in "command" (from LSB to MSB):
		// 1 = ACK, "Please respond to this packet" (using the remotePacketID). Exception: The initial 10-20 kbytes of Switcher status
		// 2 = Initialization (first hand-shake packets contains that)
		// 3 = "This is a retransmission". You will see this bit set if the ATEM switcher did not get a timely response to a packet.
		// 5 = "This is a response on your request". So set this when answering...

    if (packetSize==packetLength) {  // Just to make sure these are equal, they should be!

      // If a packet is 12 bytes long it indicates that all the initial information 
      // has been delivered from the ATEM and we can begin to answer back on every request
	  // Currently we don't know any other way to decide if an answer should be sent back...
      if(!_hasInitialized && packetSize == 12) {
        _hasInitialized = true;
      } 
	
		if (packetLength > 12)	{
			_parsePacket(packetLength);
		}

      // If we are initialized, lets answer back no matter what:
      if (_hasInitialized && command_ACK) {
        Serial.print("ACK, rpID: ");
        Serial.println(remotePacketID, DEC);

        _sendAnswerPacket(remotePacketID);
      }

    } else {
      Serial.print("ERROR: Packet size mismatch: ");
      Serial.print(packetSize, DEC);
      Serial.print(" != ");
      Serial.println(packetLength, DEC);

		// Flushing the buffer:
		// TODO: Other way? _Udp.flush() ??
          while(_Udp.available()) {
              _Udp.read(_packetBuffer, 96);
          }
    }
  }
}


void ATEM::_parsePacket(uint16_t packetLength)	{
     
 		// If packet is more than an ACK packet (= if its longer than 12 bytes header), lets parse it:
      uint16_t indexPointer = 12;
      while (indexPointer < packetLength)  {

        // Read the length of segment (first word):
        _Udp.read(_packetBuffer, 2);
        uint16_t cmdLength = word(0, _packetBuffer[1]);
			// If length of segment fits into buffer, lets read it, otherwise throw an error:
        if (cmdLength>2 && cmdLength<=96)  {

          // Read the rest of the segment:
          _Udp.read(_packetBuffer, cmdLength-2);

          // Get the "command string", basically this is the 4 char variable name in the ATEM memory holding the various state values of the system:
          char cmdStr[] = { 
            _packetBuffer[-2+4], _packetBuffer[-2+5], _packetBuffer[-2+6], _packetBuffer[-2+7], '\0'                                                  };

          // Extract the specific information we like to know about in this implementation:
          if(strcmp(cmdStr, "PrgI") == 0) {  // Program Bus status
			_ATEM_PrgI = _packetBuffer[-2+8+1];
            Serial.print("Program Bus: ");
            Serial.println(_packetBuffer[-2+8+1], DEC);
          }
          if(strcmp(cmdStr, "PrvI") == 0) {  // Preview Bus status
			_ATEM_PrvI = _packetBuffer[-2+8+1];
            Serial.print("Preview Bus: ");
            Serial.println(_packetBuffer[-2+8+1], DEC);
          }

          indexPointer+=cmdLength;
        } else { 
          // Error, just get out of the loop ASAP:
          Serial.print("ERROR: Command Size mismatch: ");
          Serial.print(cmdLength, DEC);
          indexPointer = 2000;
          
			// Flushing the buffer:
			// TODO: Other way? _Udp.flush() ??
	          while(_Udp.available()) {
	              _Udp.read(_packetBuffer, 96);
	          }
        }
      }
}


void ATEM::_sendAnswerPacket(uint16_t remotePacketID)  {

  //Answer packet:
  memset(_answer, 0, 12);			// Using 12 bytes of answer buffer, setting to zeros.
  _answer[2] = 0x80;  // ??? API
  _answer[3] = _sessionID;  // Session ID
  _answer[4] = remotePacketID/256;  // Remote Packet ID, MSB
  _answer[5] = remotePacketID%256;  // Remote Packet ID, LSB
  _answer[9] = 0x41;  // ??? API
  // The rest is zeros.

  // Create header:
  uint16_t returnPacketLength = 10+2;
  _answer[0] = returnPacketLength/256;
  _answer[1] = returnPacketLength%256;
  _answer[0] |= B10000000;

  // Send connectAnswerString to ATEM:
  _Udp.beginPacket(_switcherIP,  9910);
  _Udp.write(_answer,returnPacketLength);
  _Udp.endPacket();  
}

void ATEM::_sendCommandPacket(char cmd[4], uint8_t commandBytes[16], uint8_t cmdBytes)  {

  if (cmdBytes <= 4)	{	// Currently, only a lenght of 4 - can be extended, but then the _answer buffer must be prolonged as well (to more than 24)
	  //Answer packet preparations:
	  memset(_answer, 0, 24);
	  _answer[2] = 0x80;  // ??? API
	  _answer[3] = _sessionID;  // Session ID
	  _answer[10] = _localPacketIdCounter/256;  // Remote Packet ID, MSB
	  _answer[11] = _localPacketIdCounter%256;  // Remote Packet ID, LSB

	  // The rest is zeros.

	  // Command identifier (4 bytes, after header (12 bytes) and local segment length (4 bytes)):
	  int i;
	  for (i=0; i<4; i++)  {
	    _answer[12+4+i] = cmd[i];
	  }

  		// Command value (after command):
	  for (i=0; i<cmdBytes; i++)  {
	    _answer[12+4+4+i] = commandBytes[i];
	  }

	  // Command length:
	  _answer[12] = (4+4+cmdBytes)/256;
	  _answer[12+1] = (4+4+cmdBytes)%256;

	  // Create header:
	  uint16_t returnPacketLength = 10+2+(4+4+cmdBytes);
	  _answer[0] = returnPacketLength/256;
	  _answer[1] = returnPacketLength%256;
	  _answer[0] |= B00001000;

	  // Send connectAnswerString to ATEM:
	  _Udp.beginPacket(_switcherIP,  9910);
	  _Udp.write(_answer,returnPacketLength);
	  _Udp.endPacket();  

	  _localPacketIdCounter++;
	}
}








uint8_t ATEM::getATEM_PrgI() {
	return _ATEM_PrgI;
}
void ATEM::setATEM_PrgI(uint8_t inputNumber)  {
  // TODO: Validate that input number exists on current model!
	// On ATEM 1M/E: Black (0), 1 (1), 2 (2), 3 (3), 4 (4), 5 (5), 6 (6), 7 (7), 8 (8), Bars (9), Color1 (10), Color 2 (11), Media 1 (12), Media 2 (14)

  uint8_t commandBytes[4] = {0, inputNumber, 0, 0};
  _sendCommandPacket("CPgI", commandBytes, 4);
}


uint8_t ATEM::getATEM_PrvI() {
	return _ATEM_PrvI;
}
void ATEM::setATEM_PrvI(uint8_t inputNumber)  {
  // TODO: Validate that input number exists on current model!

  uint8_t commandBytes[4] = {0, inputNumber, 0, 0};
  _sendCommandPacket("CPvI", commandBytes, 4);
}

void ATEM::send_cut()	{
  uint8_t commandBytes[4] = {0, 0xef, 0xbf, 0x5f};	// I don't know what that actually means...
  _sendCommandPacket("DCut", commandBytes, 4);
}
