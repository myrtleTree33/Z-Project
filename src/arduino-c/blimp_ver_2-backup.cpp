#include <SoftwareSerial.h>
#include <StringSerial.h>
#include <L293D.h>

/* uncomment as appropriate */
#define TRANSMITTER
//#define RECEIVER

const unsigned int	BAUD						= 9600;
const unsigned long	PACKET_TIMEOUT					= 4000;
const char		DELIMITER					= ' ';
const char		EOL						= '%';

/********** OP CODES **********/

/******************************/


/***** TRANSMITTER *****/
#ifdef TRANSMITTER

/* Define constants */
const unsigned int 	RX_PIN 						= 2;
const unsigned int	TX_PIN						= 3;

const unsigned int	PING_INTERVAL					= 4000;
const unsigned int	PING_TIMEOUT					= 4000;


boolean			isOnline					= false;

SoftwareSerial 		softTx(RX_PIN,TX_PIN);
StringSerial		xbee(BAUD, PACKET_TIMEOUT, softTx);

unsigned long 		refTime						= 0;

boolean pingFailure() {
	unsigned long curTime = millis();
	unsigned long diff = curTime - refTime;
	if (diff < PING_INTERVAL) {
		return false;
	} else {
		boolean replied = false;
		xbee.send("PNG 206");
		unsigned long refTime2 = millis();
		unsigned long curTime2 = 0;
		unsigned long diff2 = 0;
		
		while (!replied) {
			curTime2 = millis();
			diff2 = curTime2 - refTime2;
			
			if (diff2 < PING_TIMEOUT) {
				if (xbee.parse(xbee.receive()) && xbee.header() == "ACK" && xbee.payload() == "206") {
					refTime = millis();
					Serial.println("PNG OK");
					return false;
				}

			} else
				return true;
		}
	}
}

/* This routine sends a command and waits for the ACK reply.  If it is successful, TRUE is returned. */
boolean communicate(unsigned long timeOut, String command, String responseHeader, String responsePayload) {
	unsigned long initialTime = 0, diff = 0;
	xbee.send(command);
	initialTime = millis();
	while (1) {
		diff = millis() - initialTime;
		if (diff < timeOut) {
			if (xbee.parse(xbee.receive()) && (xbee.header() == responseHeader) && (xbee.payload() == responsePayload)) {
				Serial.println("MSG success");
				return true;
			}
		} else
			return false;
	}
}
		

void setup() {
	/* set up xbee */
	xbee.init();
	xbee.setDelimiter(DELIMITER);
	xbee.setEOL(EOL);
	
	/*set up hardware UART */
	Serial.begin(BAUD);
	
	
}

void loop() {
	
	/* establish connection with Blimp */
	if (!isOnline)
		Serial.println("Waiting for Blimp to appear online...");
	
	while (!isOnline) {
		if (xbee.parse(xbee.receive()) && (xbee.header() == "BGN") ) {
			Serial.println("Blimp is online, awaiting further instruction...");
			xbee.send("ACK BGN");
			
			/* start the time to ping at intervals */
			refTime = millis();	
			isOnline = true;
		}
	}
	
	
	
	/* ping for connectivity */
	if (pingFailure()) {
		Serial.println("Blimp disconnected.  Please reconnect.");
		isOnline = false;
	}
	/*	
	xbee.send("LDR 0");
	delay(500);
	xbee.send("LDR 1");
	delay(500);*/
	communicate(4000, "LDR 0", "ACK", "LDR 0");
	delay(500);
	communicate(4000, "LDR 1", "ACK", "LDR 1");
	delay(500);
	
}

#endif


/***** RECEIVER *****/
#ifdef RECEIVER

StringSerial xbee(BAUD, PACKET_TIMEOUT);

boolean isOnline					= false;


void setup() {
	xbee.init();
	xbee.setDelimiter(DELIMITER);
	xbee.setEOL(EOL);
	pinMode(13,OUTPUT);
	
	/* establish connection first */
	while (!isOnline) {
		xbee.send("BGN 243");
		if (xbee.parse(xbee.receive()) && (xbee.header() == "ACK") && (xbee.payload() == "BGN"))
			isOnline = true;
	}
}

void loop() {
	
	
	//begin stuff here
	if (xbee.parse(xbee.receive())) {				//there is some data written
		if (xbee.header() == "PNG") {				//ping request is sent
			xbee.send("ACK " + xbee.payload());		//send the key back
						
		} else if (xbee.header() == "LDR") {
			if (xbee.payload() == "1") {
				digitalWrite(13, HIGH);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "0") {
				digitalWrite(13, LOW);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
			}
		}
	}
}

#endif

