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




/*****************
 * Example: ATEM Basic Control
 * Connects to the Atem Switcher and outputs changes to Preview and Program on the Serial monitor (at 9600 baud)
 * Uses digital inputs 2 and 3 (active High) to select input 1 and 2 on Preview Bus
 * Uses digital input 7 (active High) to "Cut" (Preview and Program swaps)
 * Uses digital outputs 4 and 5 for Tally LEDs for input 1 and 2 (Active LOW)
 * See file "Breadboard circuit for ATEM basic control.pdf" for suggested breadboard schematics (find it under ATEM/examples/ATEMbasicControl/ in this library)
 */
/*****************
 * TO MAKE THIS EXAMPLE WORK:
 * - You must have an Arduino with Ethernet Shield (or compatible such as "Arduino Ethernet", http://arduino.cc/en/Main/ArduinoBoardEthernet)
 * - You must have an Atem Switcher connected to the same network as the Arduino - and you should have it working with the desktop software
 * - You must make specific set ups in the below lines where the comment "// SETUP" is found!
 */





#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>


// MAC address and IP address for this *particular* Ethernet Shield!
// MAC address is printed on the shield
// IP address is an available address you choose on your subnet where the switcher is also present:
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0xE8, 0xE9 };		// <= SETUP
IPAddress ip(192, 168, 0, 20);				// <= SETUP


// Include ATEM library and make an instance:
#include <ATEM.h>

// Connect to an ATEM switcher on this address and using this local port:
// The port number is chosen randomly among high numbers.
ATEM AtemSwitcher(IPAddress(192, 168, 0, 50), 56417);  // <= SETUP (the IP address of the ATEM switcher)



void setup() { 

    // Set up pins for 
  pinMode(7, INPUT);  // Cut
  pinMode(2, INPUT);  // Select 1 for Preview
  pinMode(3, INPUT);  // Select 2 for Preview
  pinMode(4, OUTPUT);  // LED: 1 is on Program (Tally)
  pinMode(5, OUTPUT);  // LED: 2 is on Program (Tally)
    
  // Start the Ethernet, Serial (debugging) and UDP:
  Ethernet.begin(mac,ip);
  Serial.begin(9600);  

  // Initialize a connection to the switcher:
  AtemSwitcher.serialOutput(true);
  AtemSwitcher.connect();
}

// For pushButtons:
int pushButton = 0;

void loop() {
  
    // Check for packets, respond to them etc. Keeping the connection alive!
  AtemSwitcher.runLoop();


    // Write the Tally LEDS:
  digitalWrite(4, !AtemSwitcher.getProgramTally(1));
  digitalWrite(5, !AtemSwitcher.getProgramTally(2));


  if (digitalRead(2))  {
    if (pushButton !=2)  {
      pushButton=2;
      Serial.println("Select 1");
      AtemSwitcher.changePreviewInput(1);
    }
  } else if (pushButton==2) {
    pushButton = 0; 
  }
  if (digitalRead(3))  {
    if (pushButton !=3)  {
      pushButton=3;
      Serial.println("Select 2");
      AtemSwitcher.changePreviewInput(2);
    }
  } else if (pushButton==3) {
    pushButton = 0; 
  }
  if (digitalRead(7))  {
    if (pushButton !=7)  {
      pushButton=7;
      Serial.println("Cut");
      AtemSwitcher.doCut();
    }
  } else if (pushButton==7) {
    pushButton = 0; 
  }
}