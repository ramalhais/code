/*
  MeetAndroidStream.cpp - Arduino Library for Amarino
  Copyright (c) 2009 Bonifaz Kaufmann.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Includes
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif

#include "HardwareSerial.h"
#include <SoftwareSerial.h>
#include "MeetAndroidStream.h"

extern "C" {
#include <stdlib.h>
}


// Private methods
void MeetAndroidStream::processCommand(){
	if(buffer[0]-FunctionBufferOffset < FunctionBufferLenght){
		void (*H_FuncPtr)(uint8_t, uint8_t) = intFunc[buffer[0]-FunctionBufferOffset];
		if (H_FuncPtr != 0) {
			H_FuncPtr(buffer[0], getArrayLength());
		}
		else {
			send("Flag not registered: ");
			send(buffer[0]);
		}
	}
	else {
		if (customErrorFunc)
			errorFunc(buffer[0], getArrayLength());
		else {
			send("Flag out of bounds: ");
			send(buffer[0]);
		}
	}
}


void MeetAndroidStream::init()
{
	waitTime = 30;
	startFlag = 18;
	ack = 19;
	abord = 27;
	delimiter = 59; //';'

	numberOfValues = 0;
	
	for(int a = 0;a < FunctionBufferLenght;a++){
		intFunc[a] = errorFunc;
	}
}


// public methods
#if defined(USBCON)
MeetAndroidStream::MeetAndroidStream(): amarinoSerial(Serial1)
#else
MeetAndroidStream::MeetAndroidStream(): amarinoSerial(Serial)
#endif
{
    // it is hard to use member function pointer together with normal function pointers.
    customErrorFunc = false;
	errorFunc = 0;
	init();
}

// Constructur for use with HardwareSerial library
#if defined(USBCON)
MeetAndroidStream::MeetAndroidStream(H_voidFuncPtr err): amarinoSerial(Serial1)
#else
MeetAndroidStream::MeetAndroidStream(H_voidFuncPtr err): amarinoSerial(Serial)
#endif
{    
    customErrorFunc = true;
	errorFunc = err;
	init();
}

// Constructur for use with SoftwareSerial library
MeetAndroidStream::MeetAndroidStream(Stream &stream): amarinoSerial(stream)
{
    customErrorFunc = true;
	errorFunc = 0;
	init();
}


void MeetAndroidStream::registerFunction(void(*userfunction)(uint8_t, uint8_t),uint8_t command){
	intFunc[command-FunctionBufferOffset] = userfunction;
}
void MeetAndroidStream::unregisterFunction(uint8_t command){
	intFunc[command-FunctionBufferOffset] = errorFunc;
}

bool MeetAndroidStream::receive(){
	uint8_t lastByte;
	boolean timeout = false;
	while(!timeout)
	{
		while(amarinoSerial.available() > 0)
		{
			lastByte = amarinoSerial.read();
			
			if(lastByte == abord){
				flush();
			}
			else if(lastByte == ack){
				processCommand();
				flush();
			}
			else if(bufferCount < ByteBufferLenght){
				buffer[bufferCount] = lastByte;
				bufferCount++;
			}
			else return false;
		}
		
		if(amarinoSerial.available() <= 0 && !timeout){
			if(waitTime > 0) delayMicroseconds(waitTime);
			if(amarinoSerial.available() <= 0) timeout = true;
		}
	}
	return timeout;
}




void MeetAndroidStream::getBuffer(uint8_t buf[]){

	for(int a = 0;a < bufferCount;a++){
		buf[a] = buffer[a];
	}
}

void MeetAndroidStream::getString(char string[]){

	for(int a = 1;a < bufferCount;a++){
		string[a-1] = buffer[a];
	}
	string[bufferCount-1] = '\0';
}

int MeetAndroidStream::getInt()
{
	uint8_t b[bufferCount];
	for(int a = 1;a < bufferCount;a++){
		b[a-1] = buffer[a];
	}

	b[bufferCount-1] = '\0';
	return atoi((char*)b);
}

long MeetAndroidStream::getLong()
{
	uint8_t b[bufferCount];
	for(int a = 1;a < bufferCount;a++){
		b[a-1] = buffer[a];
	}

	b[bufferCount-1] = '\0';
	return atol((char*)b);
}

float MeetAndroidStream::getFloat()
{
	return (float)getDouble();
}

int MeetAndroidStream::getArrayLength()
{
	if (bufferCount == 1) return 0; // only a flag and ack was sent, not data attached
	numberOfValues = 1;
	// find the amount of values we got
	for (int a=1; a<bufferCount;a++){
		if (buffer[a]==delimiter) numberOfValues++;
	}
	return numberOfValues;
}

void MeetAndroidStream::getFloatValues(float values[])
{
	int t = 0; // counter for each char based array
	int pos = 0;

	int start = 1; // start of first value
	for (int end=1; end<bufferCount;end++){
		// find end of value
		if (buffer[end]==delimiter) {
			// now we know start and end of a value
			char b[(end-start)+1]; // create container for one value plus '\0'
			t = 0;
			for(int i = start;i < end;i++){
				b[t++] = (char)buffer[i];
			}
			b[t] = '\0';
			values[pos++] = atof(b);
			start = end+1;
		}
	}
	// get the last value
	char b[(bufferCount-start)+1]; // create container for one value plus '\0'
	t = 0;
	for(int i = start;i < bufferCount;i++){
		b[t++] = (char)buffer[i];
	}
	b[t] = '\0';
	values[pos] = atof(b);
}

// not tested yet
void MeetAndroidStream::getDoubleValues(float values[])
{
	getFloatValues(values);
}

// not tested yet
void MeetAndroidStream::getIntValues(int values[])
{
	int t = 0; // counter for each char based array
	int pos = 0;

	int start = 1; // start of first value
	for (int end=1; end<bufferCount;end++){
		// find end of value
		if (buffer[end]==delimiter) {
			// now we know start and end of a value
			char b[(end-start)+1]; // create container for one value plus '\0'
			t = 0;
			for(int i = start;i < end;i++){
				b[t++] = (char)buffer[i];
			}
			b[t] = '\0';
			values[pos++] = atoi(b);
			start = end+1;
		}
	}
	// get the last value
	char b[(bufferCount-start)+1]; // create container for one value plus '\0'
	t = 0;
	for(int i = start;i < bufferCount;i++){
		b[t++] = (char)buffer[i];
	}
	b[t] = '\0';
	values[pos] = atoi(b);
}


double MeetAndroidStream::getDouble()
{
	char b[bufferCount];
	for(int a = 1;a < bufferCount;a++){
		b[a-1] = (char)buffer[a];
	}

	b[bufferCount-1] = '\0';
	return atof(b);
	
}


#if defined(ARDUINO) && ARDUINO >= 100
size_t MeetAndroidStream::write(uint8_t b){
	amarinoSerial.print(b);
	return 1;
}
#else
void MeetAndroidStream::write(uint8_t b){
	amarinoSerial.print(b);
}
#endif
	


void MeetAndroidStream::send(char c ){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(c);
	amarinoSerial.print(ack);
}

void MeetAndroidStream::send(const char str[]){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(str);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::send(uint8_t n){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(n);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::send(int n){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(n);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::send(unsigned int n){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(n);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::send(long n){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(n);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::send(unsigned long n){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(n);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::send(long n, int base){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(n, base);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::send(double n){
	amarinoSerial.print(startFlag);
	amarinoSerial.print(n);
	amarinoSerial.print(ack);
}
void MeetAndroidStream::sendln(void){
	amarinoSerial.print(startFlag);
	amarinoSerial.println();
	amarinoSerial.print(ack);
}

void MeetAndroidStream::flush(){
	for(uint8_t a=0; a < ByteBufferLenght; a++){
		buffer[a] = 0;
	}
	bufferCount = 0;
	numberOfValues = 0;
}
