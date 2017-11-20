/**
	OLTC Control, adapted for the co-simulation based on FMITerminalBlock made by AIT
	this Project is promoted and Funded	by EU within EriGRID Consortium.
	
**/

	#include "MgsModbus.h"
	#include <SPI.h>
	#include <Ethernet.h>
	#include <SD.h>
	Sd2Card card;
	File dataFile;
	
/** Ethernet2 settings (depending on MAC and Local network) **/
	byte mac[]  =   {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5 };
	IPAddress	ip    	(10		, 2		, 173	,15	);
	IPAddress	gateway (10		, 2		, 173	,1	);
	IPAddress	subnet  (255	, 255	, 255	,0	);
	
	MgsModbus	Mb			 		;		// declaration of a modbus Variable
	int			inByte		=	0	;		// incoming serial byte
	float		i			=	0	;
	boolean		f			=	1	;		//Flag para la inicializacion inicial de los registros
	
/** Average calculation **/
	const int	numReadings 	=	10 ;
	float		readings[numReadings]  ;	// the readings from the analog input
	int			readIndex 		= 	 0 ;	// the index of the current reading
	float		total 			=	 0 ;	// the running total
	float		average 		=	 0 ;	// the average
	int			inputVoltage	=	A0 ;	//	inputPin
	int			InputTAP		=	A1 ;	//	inputPin
	

/** definir la Entradas de control	**/
	const	int	ReadyPin	=	2	; 
			int	TAP			=  36	;		// Tap position,Coming from Transformer OLTC. and displayed in Mb Holding register.
	//	boolean		Ready	 = 1 ;

/** definir la Salidas de control	**/
	String	Status	= "Not Ready.."	;
	float	SetP	= 700	;				// SetPoint to map the input value to (0..1023 ==> 0..SetP)
	int		Position= 	8	;

/**	definir las salidas intermedias	**/
	float	CBLQ_H ;
	float	CQCK_H ;
	float	CNOR_H ;
	float	CTOL_H ;
	float	CTOL_L ;
	float	CNOR_L ;
	float	CQCK_L ;
	float	CBLQ_L ;
	
/**				SETUP				**/
void setup()
{
	Serial.begin(9600);
	while (!Serial) {	;}					// wait for serial port to connect. Needed for native USB port only

/**				I/O*				**/
	pinMode(ReadyPin, INPUT);
	
/**			SD CARD					**/
	Serial.print("Initializing SD card...");
	if (!card.init(SPI_HALF_SPEED, 4))	
		{	Serial.println("SD Card failed..");	return;	}
	else
		{	Serial.println("SD Card OK!");}
	
/**			IP						**/
	// 	initialize the Ethernet2 device
	Ethernet.begin(mac, ip, gateway, subnet);   
	
	// 	start etehrnet interface
	Serial.println("Ethernet interface started");
	
	initialiseModbusRegs() ;
	Serial.println("Modbus initialised");
	
	// 	print your local IP address:
	Serial.println("Arduino IP address: ");
	for (byte thisByte = 0; thisByte < 4; thisByte++) 
		{
	//	print the value of each byte of the IP address:
		Serial.print(Ethernet.localIP()[thisByte], DEC);
		Serial.print(".");
		}      Serial.println("");
	}
	
	void initialiseModbusRegs() 
	{  
		Mb.MbData[ 0] =	0      	;
		Mb.MbData[ 1] =	SetP   	;
		Mb.MbData[20] =	99		;					//reset FMU input.
		Mb.MbData[21] =	0		;					//reset Switch between FMU/physical input.
		
		resetBands();
	}
	
	void resetBands()
	{
		CBLQ_H = Mb.MbData[ 1] * (1 - 0.05);		//CBLQ_H = Mb.MbData[ 1] * (1 + 0.30);
		CQCK_H = Mb.MbData[ 1] * (1 - 0.10);		//CQCK_H = Mb.MbData[ 1] * (1 + 0.20);	
	//	CNOR_H = Mb.MbData[ 1] * (1 - 0.15);		//CNOR_H = Mb.MbData[ 1] * (1 + 0.10);	
		CTOL_H = Mb.MbData[ 1] * (1 - 0.20);		//CTOL_H = Mb.MbData[ 1] * (1 + 0.05);	
		CTOL_L = Mb.MbData[ 1] * (1 - 0.25);		//CTOL_L = Mb.MbData[ 1] * (1 - 0.05);	
	//	CNOR_L = Mb.MbData[ 1] * (1 - 0.30);		//CNOR_L = Mb.MbData[ 1] * (1 - 0.10);	
		CQCK_L = Mb.MbData[ 1] * (1 - 0.35);		//CQCK_L = Mb.MbData[ 1] * (1 - 0.20);	
		CBLQ_L = Mb.MbData[ 1] * (1 - 0.40);		//CBLQ_L = Mb.MbData[ 1] * (1 - 0.30);	
				
	//	Mb.MbData[ 2] =	0      	;
		Mb.MbData[ 3] = CBLQ_H	;
		Mb.MbData[ 4] = CQCK_H	;
		Mb.MbData[ 5] = CTOL_H	;
		Mb.MbData[ 6] =	0		;    
		Mb.MbData[ 7] = CTOL_L	;
		Mb.MbData[ 8] = CQCK_L	;
		Mb.MbData[ 9] = CBLQ_L	;
		Mb.MbData[10] =	0      	;
		
		
	}
	
/**				LOOP				**/
void loop()
{
		delay(1000);
		resetBands();
		Mb.MbData[ 0] = i;i++;
		Mb.MbData[12] = digitalRead(ReadyPin);	// Ready input
		Mb.MbData[11] =  map(analogRead(InputTAP), 0, 1023, 0, 8); // Taps Output

/**		Average calculation		**/
	total = total - readings[readIndex];
	readings[readIndex] = analogRead(inputVoltage);
	total = total + readings[readIndex];
	readIndex++;
	if (readIndex >= numReadings)  readIndex = 0;
	if (Mb.MbData[21] ==1)	{average=Mb.MbData[20];}
			else			{average= Mb.MbData[ 1] * (total / numReadings) / 1023;}
/*****************SD**********************/
	
	int InstVolt = analogRead(inputVoltage);
	InstVolt = map(InstVolt, 0, 1023, 0, Mb.MbData[ 1]);
	
// 	put your main code here, to run repeatedly:
//*********************************************************
	if (Mb.MbData[12])
	{
			if						 (average > CBLQ_H)	{Status = "Over Voltage__________________"; Position =  1 ; } // Blocked
	else	if	(average <= CBLQ_H && average > CQCK_H)	{Status = "Fast action Step-Up needed____"; Position =  6 ; } // Step++
	else	if	(average <= CQCK_H && average > CTOL_H)	{Status = "Normal Step-Up needed_________"; Position =  4 ; } // Step+
	else	if	(average <= CTOL_H && average > CTOL_L)	{Status = "Voltage normal________________"; Position =  8 ; } // Normal
	else	if	(average <= CTOL_L && average > CQCK_L)	{Status = "Normal Step-Down needed_______"; Position = 16 ; } // Step-
	else	if	(average <= CQCK_L && average > CBLQ_L)	{Status = "Fast action Step-Down needed__"; Position = 48 ; } // Step--
	else	if	(average <= CBLQ_L)                    	{Status = "Under Voltage_________________"; Position = 64 ; } // Blocked
	}
//*********************************************************
		Mb.MbData[10]=  		Position 	; // Blocked
		Mb.MbData[13]=  bitRead(Position,6) ; // Blocked
		Mb.MbData[14]=  bitRead(Position,5) ; // Step++
		Mb.MbData[15]=  bitRead(Position,4) ; // Step+
		Mb.MbData[16]=  bitRead(Position,3) ; // Normal
		Mb.MbData[17]=  bitRead(Position,2) ; // Step-
		Mb.MbData[18]=  bitRead(Position,1) ; // Step--
		Mb.MbData[19]=  bitRead(Position,0) ; // Blocked
	
/*****************MB**********************/
		Mb.MbData[ 2] = InstVolt;
		Mb.MbData[ 6] = average;
		Mb.MbsRun();
/*****************MB**********************/
	Serial.print("| "); Serial.print(InstVolt);	Serial.print("\t|   "); Serial.print(average);  Serial.print("\t| "); Serial.print(Status);
	
	dataFile = SD.open("datalog.txt", FILE_WRITE);
	if (dataFile) 
		{
		dataFile.println(average);
		dataFile.close();
		// print to the serial port too:
		Serial.print("(");
		Serial.print(average);
		Serial.println(")⇒SD");
		}
// if the file isn't open, pop up an error:
	else	{Serial.print("\nerror opening datalog.txt |");}
}
	
// 12 hours 		= 43200 seconds
// Modbus Registers = 65535 = 18:12:15.
	