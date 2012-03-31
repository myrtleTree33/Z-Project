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

/* stored in the directional state flag */
const unsigned int	OP_STOP						= 0;
const unsigned int	OP_MOVE_FWD					= 1;
const unsigned int	OP_MOVE_REV					= 2;
const unsigned int	OP_TURN_LEFT					= 3;
const unsigned int	OP_TURN_RIGHT					= 4;
const unsigned int	OP_STRAFE_FWD_LEFT				= 5;
const unsigned int	OP_STRAFE_FWD_RIGHT				= 6;
const unsigned int	OP_STRAFE_REV_LEFT				= 7;
const unsigned int	OP_STRAFE_REV_RIGHT				= 8;

/******************************/



/***** TRANSMITTER *****/
#ifdef TRANSMITTER

/* FLAGS */
boolean 		FLAG_LED_STATE					= false;		//LED state
unsigned int		FLAG_MOTOR_ABSTRACT_STATE			= OP_STOP;		//abstraction of the motor class


/* Define constants */
const unsigned int 	RX_PIN 						= 2;
const unsigned int	TX_PIN						= 3;
const unsigned int	COMP_BAUD					= 9600;

const unsigned int	PING_INTERVAL					= 4000;
const unsigned int	PING_TIMEOUT					= 4000;


boolean			isOnline					= false;

SoftwareSerial 		softTx(RX_PIN,TX_PIN);
StringSerial		xbee(BAUD, PACKET_TIMEOUT, softTx);

unsigned long 		refTime						= 0;
unsigned long		keyLastPressed					= 0;
char			keyPressed					= '|';			//this has to be global to prevent baud lag in keypress sensitivity.  A non-mapped key is used for initialization.


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

/* This routine sends a command and waits for the ACK reply.  If it is successful, TRUE is returned. 
 * To avoid checking for payload accuracy, pass a "*" string to responsePayload
 */
boolean communicate(unsigned long timeOut, String command, String responseHeader, String responsePayload) {
	unsigned long initialTime = 0, diff = 0;
	xbee.send(command);
	initialTime = millis();
	while (1) {
		diff = millis() - initialTime;
		if (diff < timeOut) {
			if (xbee.parse(xbee.receive()) && (xbee.header() == responseHeader) ) {
				if (responsePayload != "*" && xbee.payload() == responsePayload) {
					Serial.println("MSG success");
					return true;
					
				} else if (responsePayload == "*")
					return true;
			}
		} else
			return false;
	}
}


/* IMPT: This routine resets all flags to default */
void resetFlags() {
	FLAG_LED_STATE = false;
	FLAG_MOTOR_ABSTRACT_STATE = OP_STOP;
}


void setup() {
	/* reset flags */
	resetFlags();

	/* set up xbee */
	xbee.init();
	xbee.setDelimiter(DELIMITER);
	xbee.setEOL(EOL);
	
	/*set up hardware UART */
	Serial.begin(COMP_BAUD);
		
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
		resetFlags();									//reset all flags
		isOnline = false;
	}
	/*
	communicate(4000, "LDR 0", "ACK", "LDR 0");
	delay(500);
	communicate(4000, "LDR 1", "ACK", "LDR 1");
	delay(500);
	*/
	
	/* begin the key tracking sequence */
	if (Serial.available()) {
		keyPressed = Serial.read();
		
		if ((keyPressed == ']') && 
			(!FLAG_LED_STATE) && 
			(communicate(PACKET_TIMEOUT, "LDR 1", "ACK", "LDR 1"))) {
			
			FLAG_LED_STATE = true;
			
		} else if ((keyPressed == '[') && 
			(FLAG_LED_STATE) && 
			(communicate(PACKET_TIMEOUT, "LDR 0", "ACK", "LDR 0"))) {	
			
			FLAG_LED_STATE = false;
			
		} else if ((keyPressed == 'p') &&  
			(communicate(PACKET_TIMEOUT, "REQ UPT", "UPT", "*"))) {	
			Serial.println( xbee.payload() );
			
		} else if (keyPressed == 'w') {
			keyLastPressed = millis();	//this is to allow some lag time to register next keypress and not list it as a key-not-pressed event.  Even if forward flag is set.
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_MOVE_FWD) && (communicate(PACKET_TIMEOUT, "MOV FWD", "ACK", "MOV FWD"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_MOVE_FWD;
				
			}
			
		}  else if (keyPressed == 's') {
			keyLastPressed = millis();
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_MOVE_REV) && (communicate(PACKET_TIMEOUT, "MOV REV", "ACK", "MOV REV"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_MOVE_REV;
				
			}
			
		} else if (keyPressed == 'a') {
			keyLastPressed = millis();
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_TURN_LEFT) && (communicate(PACKET_TIMEOUT, "MOV LEFT", "ACK", "MOV LEFT"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_TURN_LEFT;
				
			}
			
		} else if (keyPressed == 'd') {
			keyLastPressed = millis();
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_TURN_RIGHT) && (communicate(PACKET_TIMEOUT, "MOV RIGHT", "ACK", "MOV RIGHT"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_TURN_RIGHT;
				
			}
			
		} else if (keyPressed == 'q') {
			keyLastPressed = millis();
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_STRAFE_FWD_LEFT) && (communicate(PACKET_TIMEOUT, "MOV STRAFE_FWD_LEFT", "ACK", "MOV STRAFE_FWD_LEFT"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_STRAFE_FWD_LEFT;
				
			}
			
		} else if (keyPressed == 'e') {
			keyLastPressed = millis();
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_STRAFE_FWD_RIGHT) && (communicate(PACKET_TIMEOUT, "MOV STRAFE_FWD_RIGHT", "ACK", "MOV STRAFE_FWD_RIGHT"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_STRAFE_FWD_RIGHT;
				
			}
			
		} else if (keyPressed == 'z') {
			keyLastPressed = millis();
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_STRAFE_REV_LEFT) && (communicate(PACKET_TIMEOUT, "MOV STRAFE_REV_LEFT", "ACK", "MOV STRAFE_REV_LEFT"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_STRAFE_REV_LEFT;
				
			}
			
		} else if (keyPressed == 'c') {
			keyLastPressed = millis();
			if( (FLAG_MOTOR_ABSTRACT_STATE != OP_STRAFE_REV_RIGHT) && (communicate(PACKET_TIMEOUT, "MOV STRAFE_REV_RIGHT", "ACK", "MOV STRAFE_REV_RIGHT"))) {
				FLAG_MOTOR_ABSTRACT_STATE = OP_STRAFE_REV_RIGHT;
				
			}
			
		} 
			
	} else {
		if (FLAG_MOTOR_ABSTRACT_STATE != OP_STOP) {
			unsigned long diff3 = millis() - keyLastPressed;
			if (diff3 > 100) {
				if (communicate(PACKET_TIMEOUT, "MOV STOP", "ACK", "MOV STOP")) {
					FLAG_MOTOR_ABSTRACT_STATE = OP_STOP;
					Serial.println("stopping now");
				}
				
			}
		}
	}
			
	
}

#endif


/***** RECEIVER *****/
#ifdef RECEIVER

/*** GLOBAL CONSTANTS ***/
const unsigned int	ELEVATOR_MOTOR_PIN		= 12;
const float 		ELEVATOR_MIN_HEIGHT		= 100;
const float 		ELEVATOR_GRACE			= 10;

/*** GLOBAL FLAGS ***/
boolean 		heightMotorOn			= false;
boolean 		isOnline			= false;

StringSerial	xbee(BAUD, PACKET_TIMEOUT);
L293D 		l293d(5,6,7,10,9,8);						//directional motor driver

unsigned long 		timeLastPing			= 0;			//time since last ping

void resetAll() {
	l293d.stop();
	digitalWrite(ELEVATOR_MOTOR_PIN, LOW);
}

void checkHeight(float minHeight, float grace) {
	float input = analogRead(0) / 2 * 2.54;
	if ( (input < (minHeight - grace)) && !heightMotorOn ) {
		digitalWrite(ELEVATOR_MOTOR_PIN, HIGH);
		heightMotorOn = true;
		
	} else if ( (input > (minHeight + grace)) && heightMotorOn ) {
		digitalWrite(ELEVATOR_MOTOR_PIN,LOW);
		heightMotorOn = false;
	}
}

/* this routine checks for time of last ping. */
boolean chkIsOnline() {
	unsigned long timeLastPingDiff = millis() - timeLastPing;
	
	if (timeLastPingDiff < (PACKET_TIMEOUT * 2) )	{		//timeout interrupt is raised after 8 secs
		return true;
	} else {
		
		return false;						//blimp has lost connection with transmitter
	}
}

void setup() {
	xbee.init();
	xbee.setDelimiter(DELIMITER);
	xbee.setEOL(EOL);
		
	pinMode(ELEVATOR_MOTOR_PIN,OUTPUT);			//for the elevator motor switch
	digitalWrite(ELEVATOR_MOTOR_PIN,LOW);			//set elevator motor pin to default as OFF.
	timeLastPing = millis();				//reset the timer last ping counter
}

void loop() {
	
	/* establish connection first */
	while (!isOnline) {
		unsigned long repeatCommandTimer = millis();
		while (1) {
			if ( (millis() - repeatCommandTimer) > 1000) {			//send connect command every 250ms to prevent buffer flooding
				xbee.send("BGN 243");
				repeatCommandTimer = millis();				//reset the timer
			}
							
			if (xbee.parse(xbee.receive()) && (xbee.header() == "ACK") && (xbee.payload() == "BGN")) {
				timeLastPing = millis();				//reset the timer last ping counter
				isOnline = true;					//break out of loop and carry on with stuff
				break;
			}
				
		}		
	}
	
	
	//checkHeight(ELEVATOR_MIN_HEIGHT, ELEVATOR_GRACE);		//regulate height at certain level
	if (!chkIsOnline() ) {						//check for time of last ping.  If exceeded, stop and reset everything.
		resetAll();
		isOnline = false;					//go back to authenticating
	}
		
	//begin stuff here
	if (xbee.parse(xbee.receive())) {				//there is some data written
		if (xbee.header() == "PNG") {				//ping request is sent
			timeLastPing = millis();			//reset the ping counter
			xbee.send("ACK " + xbee.payload());		//send the key back
			
		
		} else if (xbee.header() == "REQ") {
			if (xbee.payload() == "UPT") {
				xbee.send("UPT data sent");
				
			}
			
							
		} else if (xbee.header() == "LDR") {
			if (xbee.payload() == "1") {
				digitalWrite(ELEVATOR_MOTOR_PIN, HIGH);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "0") {
				digitalWrite(ELEVATOR_MOTOR_PIN, LOW);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
			}
			
			
		} else if (xbee.header() == "MOV") {
			if (xbee.payload() == "STOP") {
				l293d.stop();
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "FWD") {
				l293d.fwd(255);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "REV") {
				l293d.rev(255);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "LEFT") {
				l293d.setM1(255, -1);
				l293d.setM2(255, 1);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "RIGHT") {
				l293d.setM1(255, 1);
				l293d.setM2(255, -1);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "STRAFE_FWD_LEFT") {
				l293d.setM1(0, 0);
				l293d.setM2(255, 1);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "STRAFE_FWD_RIGHT") {
				l293d.setM1(255, 1);
				l293d.setM2(0, 0);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "STRAFE_REV_LEFT") {
				l293d.setM1(0, 0);
				l293d.setM2(255, -1);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} else if (xbee.payload() == "STRAFE_REV_RIGHT") {
				l293d.setM1(255, -1);
				l293d.setM2(0, 0);
				xbee.send("ACK " + xbee.header() + " " + xbee.payload());
				
			} 
		}
	}
}

#endif

