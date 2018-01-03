// ---------------------------------------- library includes--------------------------------------------------------------------
#include <mcp_can.h>
#include <SPI.h>
#include <string.h>
//---------------------------------------- Intial (One time) settings------------------------------------------------------------
// Set CS Pin
const int SPI_CS_PIN = 9;       // the cs pin of the version after v1.1 is default to D9.v0.9b and v1.0 is default D10
MCP_CAN CAN(SPI_CS_PIN);

void setup() 
{
  // Intialise Serial1
  Serial1.begin(19200);

  // Initialise CAN: Baud Rate = 250 kbps
  // DEBUG Mode: Initialise Serial and CAN
  Serial.begin(19200);
  while (CAN_OK != CAN.begin(CAN_250KBPS))
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  // Print welcome msg on console
  Serial.println("CAN BUS Shield init ok!");
  Serial.println("----All set. System ready, awaiting command/query from PUMA----");
}
//------------------------------------------ Global variables declaration---------------------------------------------------
int loopCounter = 0;
int MXRM = 0;
int MXRPT = 0;
int MXRFF = 0;
int flow = 0;
int METH=0;
int CNG=0;
int PROP=0;
int ETH=0;
//------------------------------------------ MAIN LOOP-----------------------------------------------------------------------
void loop() 
{
  // Declare and initialize ID, data length and data buffer
  unsigned int ID = 0;
  unsigned char len = 0;
  unsigned char buf[2]= {0, 0}; //We are using 2 bytes for flow, and 1 byte for everything else
  int mfChange = 0;
  unsigned char error[1] = {0};
  CAN.sendMsgBuf(0x72, 0, 1, error);
//------------------------------------------ Commands code-------------------------------------------------------------------
  // If CAN msg is available:
  if(CAN_MSGAVAIL == CAN.checkReceive())            // Check if command comes from PUMA
  {
    // read the ID,length and data
    CAN.readMsgBuf(&len, buf);
    ID = CAN.getCanId();
  //---------- Identify PUMA command using the CAN frame ID and send appropriate serial command to Gas Mixer------------
    switch (ID)
    {
      case 101: //Command: Mixer Run Mode
      if (buf[0] != MXRM)
      {
        MXRM = buf[0];
        Serial.println("Case 101- MXRM\n");
        Serial1.write("A MXRM ");
        Serial1.print(String(MXRM));
        Serial1.print("\r");
      }
      break;

      case 103: //Command: Target Pressure        
      if (buf[0] != MXRPT)
      {
        MXRPT = buf[0];
        Serial.println("Case 103- MXRPT\n");
        Serial1.write("A MXRPT ");
        Serial1.print(String(MXRPT));
        Serial1.print("\r");
      }
      break;

      case 105: //Command: Flow Rates
      flow = 256*buf[1] + buf[0];
      if (flow != MXRFF)
      {
        MXRFF = flow;
        Serial.println("Case 105- MXRFF\n");
        Serial1.write("A MXRFF ");
        Serial1.print(String(MXRFF));
        Serial1.print("\r");
      }
      break;

      case 107: //Command: Methane Mass Fractions
      if (buf[0] != METH)
      {
        METH = buf[0];
        mfChange = 1;
        Serial.println("Case 107- Meth\n");
      }
      break;

      case 109: //Command: CNG Mass Fractions
      if (buf[0] != CNG)
      {
        CNG = buf[0];
        mfChange = 1;
        Serial.println("Case 109- CNG\n");
      }
      break;

      case 111: //Command: Propane Mass Fractions
      if (buf[0] != PROP)
      {
        PROP = buf[0];
        mfChange = 1;
        Serial.println("Case 111- Prop\n");
      }
      break;
      
      case 113: //Command: Ethane Mass Fractions
      if (buf[0] != ETH)
      {
        ETH = buf[0];
        mfChange = 1;
        Serial.println("Case 113- Eth\n");
      }
      break;
    }
  //-------Send mass fraction change command to gas mixer of any of the four MFs is changed and sum of all MFs is 100%------
    if ((mfChange == 1) && (METH+ETH+PROP+CNG == 100))
    {
      Serial1.write("A MXMF ");
      Serial1.print(String(METH));
      Serial1.write("0000000 ");
      Serial1.print(String(CNG));
      Serial1.write("0000000 ");
      Serial1.print(String(PROP));
      Serial1.write("0000000 ");
      Serial1.print(String(ETH));
      Serial1.write("0000000 ");
      Serial1.print("\r");
      String mfToConsole = "A MXMF " + String(METH) + "0000000 " + String(CNG) + "0000000 " + String(PROP) + "0000000 " + String(ETH) + "0000000\r";
      Serial.println(mfToConsole); 
    }
  }
//--------------------------------- Flush Serial buffer and publish any error from Gas Mixer to PUMA-------------------  
  while (Serial1.available())
  {
    char flushBuffer = Serial1.read();
    if (flushBuffer == '?')
    {
      error[0] = {1};
      CAN.sendMsgBuf(0x72, 0, 1, error);
    }
    else
    {
      error[0] = {0};
    }    
  }
//------------------------------------------------- Measurements code------------------------------------------------------- 
 if (loopCounter % 100 == 0)
 {
 //--------------------------- Send query to Gas mixer and read the response------------------------------------------------
  Serial1.write("A\r");
  String serialString = Serial1.readString();
  
 //-------------Extract measurement data like mass flow and gage pressure from the response into a string array------------
 //********************************************* CAUTION*********************************************************************
 // ---This part of the code totally epends upon the architecture of the response frame from the Gas Mixer and may change------
 // ------------ firmware change on the Gas mixer. But it should be fairly easy to adapt the code accordingly.-----------------
 //***************************************************************************************************************************
  
  String serialDiscreteString[7];
  int plusMarker = -1;
  for (int i=0;i<7;i++)
  {
    plusMarker = serialString.indexOf('+', plusMarker + 1);
    if (i<2)
    {
      serialDiscreteString[i] = serialString.substring(plusMarker + 1, serialString.indexOf('+', plusMarker + 1) - 1);
    }
    else
    {
      serialDiscreteString[i] = serialString.substring(plusMarker + 1, serialString.indexOf('-', plusMarker + 1) - 1);
    }
  }
 //------------------------------------ Format the measurements into CAN frames----------------------------------------------
  unsigned char gagePressure[1]={0};
  unsigned char massFlow0[2]={0,0};
  unsigned char massFlow1[2]={0,0};
  unsigned char massFlow2[2]={0,0};
  unsigned char massFlow3[2]={0,0};
  unsigned char massFlow4[2]={0,0};
  gagePressure[0] = serialDiscreteString[0].toInt();
  massFlow0[0] = serialDiscreteString[1].toInt()%256;
  massFlow0[1] = serialDiscreteString[1].toInt()/256;
  massFlow1[0] = serialDiscreteString[3].toInt()%256;
  massFlow1[1] = serialDiscreteString[3].toInt()/256;
  massFlow2[0] = serialDiscreteString[4].toInt()%256;
  massFlow2[1] = serialDiscreteString[4].toInt()/256;
  massFlow3[0] = serialDiscreteString[5].toInt()%256;
  massFlow3[1] = serialDiscreteString[5].toInt()/256;
  massFlow4[0] = serialDiscreteString[6].toInt()%256;
  massFlow4[1] = serialDiscreteString[6].toInt()/256;
 //--------------------------------Send measurement data to PUMA over CAN using appropriate CAN IDs----------------------------
  CAN.sendMsgBuf(0x66, 0, 1, gagePressure);
  CAN.sendMsgBuf(0x68, 0, 2, massFlow0);
  CAN.sendMsgBuf(0x6A, 0, 2, massFlow1);
  CAN.sendMsgBuf(0x6C, 0, 2, massFlow2);
  CAN.sendMsgBuf(0x6E, 0, 2, massFlow3);
  CAN.sendMsgBuf(0x70, 0, 2, massFlow4);
 }
 loopCounter = loopCounter + 1;

}
