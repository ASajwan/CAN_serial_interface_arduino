// Include SPI and CAN libs
#include <mcp_can.h>
#include <SPI.h>
#include <string.h>

// Set CS Pin
const int SPI_CS_PIN = 9;       // the cs pin of the version after v1.1 is default to D9.v0.9b and v1.0 is default D10
MCP_CAN CAN(SPI_CS_PIN);

void setup() 
{
  // Intialise Serial1
  Serial1.begin(19200);

  // Initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialise CAN: Baud Rate = 500 kbps
  //DEBUG Mode: Initialise Serial and Print welcome msg on console
  Serial.begin(19200);
  while (CAN_OK != CAN.begin(CAN_250KBPS))
  {
       Serial.println("CAN BUS Shield init fail");
       Serial.println(" Init CAN BUS Shield again");
       delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");
  Serial.println("----All set. System ready, awaiting command/query from PUMA----");
}

int i=0;
int MXRM=0;
int MXRPT=0;
int MXRFF=0;
int METH=0;
int CNG=0;
int PROP=0;
int ETH=0;
int flow=0;

void loop() 
{
    // Declare and initialize ID, data length,data buffer, sent variables
    unsigned int ID = 0;
    unsigned char len = 0;
    unsigned char buf[2]= {0, 0}; //We are using 2 bytes for flow, and 1 byte for everything else
    int change_mf=0;
    
    
//------------------------------------------------------------ Commands code---------------------------------------------------------------------------------------    
    // If CAN msg is available:
    if(CAN_MSGAVAIL == CAN.checkReceive())            // Check if command comes from PUMA
    {
      // read the ID,length and data
      CAN.readMsgBuf(&len, buf);
      ID = CAN.getCanId();
      
      delay(1);
      // Convert data to int for fast comparing
      
//      Serial.print(buf[0]);
//      Serial.print("\n");
      
      // Check which command it is and send appropriate command over Serial1
      // Update 'sent' variable to 1 and blink LED once
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
//-----------------------------------------------DEBUG CODE------------------------------------------------------------------------------------------
//          Serial.print("A MXRM ");
//          Serial.print(String(MXRM));
//          Serial.print("\r");
//--------------------------------------------DEBUG CODE--------------------------------------------------------------------------------------------
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
//-----------------------------------------------DEBUG CODE------------------------------------------------------------------------------------------
//          Serial.print("A MXRPT ");
//          Serial.print(String(MXRPT));
//          Serial.print("\r");
//--------------------------------------------DEBUG CODE--------------------------------------------------------------------------------------------
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
//-----------------------------------------------DEBUG CODE------------------------------------------------------------------------------------------
//          Serial.print("A MXRFF ");
//          Serial.print(String(MXRFF));
//          Serial.print("\r");
//--------------------------------------------DEBUG CODE--------------------------------------------------------------------------------------------
        }
        break;
        
        case 107: //Command: Methane Mass Fractions
        
        if (buf[0] != METH)
        {
          METH = buf[0];
          change_mf=1;
          Serial.println("Case 107- Meth\n");
        }
        break;

        case 109: //Command: CNG Mass Fractions
        
        if (buf[0] != CNG)
        {
          CNG = buf[0];
          change_mf=1;
          Serial.println("Case 109- CNG\n");
        }
        break;

        case 111: //Command: Propane Mass Fractions
        
        if (buf[0] != PROP)
        {
          PROP = buf[0];
          change_mf=1;
          Serial.println("Case 111- Prop\n");
        }
        break;

        case 113: //Command: Ethane Mass Fractions
        
        if (buf[0] != ETH)
        {
          ETH = buf[0];
          change_mf=1;
          Serial.println("Case 113- Eth\n");
        }
        break; 
            
     }
     if ((change_mf==1) && (METH+ETH+PROP+CNG==100))
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
          Serial.println("MXMF\n");
//-----------------------------------------------DEBUG CODE------------------------------------------------------------------------------------------
//          Serial.print("A MXMF ");
//          Serial.print(String(METH));
//          Serial.print("0000000 ");
//          Serial.print(String(ETH));
//          Serial.print("0000000 ");
//          Serial.print(String(PROP));
//          Serial.print("0000000 ");
//          Serial.print(String(CNG));
//          Serial.print("0000000 ");
//          Serial.print("\r");
//--------------------------------------------DEBUG CODE--------------------------------------------------------------------------------------------
     }
    }

    // Flushing Serial buffer and sending error message to PUMA, if any

    while (Serial1.available())
    {
      char c = Serial1.read();
      unsigned char err[1] = {0};
      if (c=="?")
      {
        err[0] = {1};
        CAN.sendMsgBuf(0x6A, 0, 1, err);
        delay(1000);
      }
      else
      {
        err[0] = {0};
        CAN.sendMsgBuf(0x6A, 0, 1, err);
      }
    } 
      
//------------------------------------------Measurement code------------------------------------------------------------------------------------------
        // Collect measurement data from mixer at 1 Hz

        if (i%1000 == 0) // Decimate loop rate by 10
        {
          Serial1.write("ADV 1 6 5\r"); // Send query for measurement data to mixer
          delay(1);
          
          String meas_str="";
          while (Serial1.available()) // Check for response from mixer
          {
            meas_str += String(char(Serial1.read()));
          }
// ------------------------------------------Debug code----------------------------------------------------------------------------
//          Serial.print("meas_str");
//          Serial.print(meas_str);
//          Serial.print("\n");
//-----------------------------------------------------------------------------------------------------------------------------------     
         
          int first_plus = meas_str.indexOf('+');
          String gp_str= meas_str.substring(first_plus+1,meas_str.indexOf('.'));
          String mf_str= meas_str.substring(meas_str.indexOf('+',first_plus+1)+1,meas_str.length()-1);

// -------------------------------------------- Debug code----------------------------------------------------------------------------------
//          String gp_str="";
//          String mf_str="";
//          gp_str = "92";
//          mf_str = "40";
//          Serial.print("gp_str");
//          Serial.print(gp_str);
//          Serial.print("\n");
//          Serial.print("mf_str");
//          Serial.print(mf_str);
//          Serial.print("\n");
//--------------------------------------------------------------------------------------------------------------------------------------------          
           //Send measurements over CAN
          unsigned char gage_pressure[1]={0};
          unsigned char mass_flow[1]={0};
          

          gage_pressure[0] = gage_pressure[0] + gp_str.toInt();
          mass_flow[0] = mass_flow[0] + mf_str.toInt();
          

         CAN.sendMsgBuf(0x66, 0, 1, gage_pressure);
         CAN.sendMsgBuf(0x68, 0, 1, mass_flow);

        }
        
  i= i+1;
  delay(5);
}

