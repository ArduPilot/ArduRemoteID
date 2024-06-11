#include <Arduino.h>
#include <opendroneid.h>


struct ODID_BasicID_data *BasicIDPointer;
struct ODID_Location_data *LocationPointer;
struct ODID_Auth_data *AuthPointer;
struct ODID_SelfID_data *SelfIDPointer;
struct ODID_System_data *SystemPointer;
struct ODID_OperatorID_data *OperatorIDPointer;

void Send_RemoteID_Serial(ODID_UAS_Data UAS_data)
{
	//Serial.print(UAS_data.Location.Latitude, 6);
	//Serial.print(0x55);
	//struct ODID_Location_data *LocationPointer;
	BasicIDPointer = &UAS_data.BasicID[0];
	LocationPointer = &UAS_data.Location;
	//AuthPointer = &UAS_data.Auth[0];
	SelfIDPointer = &UAS_data.SelfID;
	SystemPointer = &UAS_data.System;
	OperatorIDPointer = &UAS_data.OperatorID;

	printBasicID_data(BasicIDPointer);
	printLocation_data(LocationPointer);
	printSelfID_data(SelfIDPointer);
	printSystem_data(SystemPointer);
	printOperatorID_data(OperatorIDPointer);

}
