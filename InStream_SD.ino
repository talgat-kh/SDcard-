#include "../system/driverlib/sdhost.h"
#include "../system/driverlib/timer.h"
#include "../system/driverlib/prcm.h"

void setup() {
  //Start the serial port at a baudrate of 9600
  Serial.begin(9600);
  delay(5000);
  //Below is the CODE to Setup the SDHOST Peripheral
  //Start the peripheral clock for the SDHost
  PRCMPeripheralClkEnable(PRCM_SDHOST,PRCM_RUN_MODE_CLK);

  //Define the Pins associated with the SD Card. You can reference these on the Schematic and the Pin Mode in the CC3200 Data Sheet.
  //SDCard_Clk Pin Define
  MAP_PinTypeSDHost(PIN_01, PIN_MODE_6);
  //SDCard_CMD Pin Define
  MAP_PinTypeSDHost(PIN_02, PIN_MODE_6);
  //SDCard_DATA0 Pin Define
  MAP_PinTypeSDHost(PIN_64, PIN_MODE_6);

  //Set the clock pin as an output
  PinDirModeSet(PIN_01, PIN_DIR_MODE_OUT);

  //Soft Reset and initialize the SD Host Controller
  //Talgat I am unsure if the SDHOST_BASE register is correct.  There may be an additional character to describe which BASE is linked to our pins
  PRCMPeripheralReset(PRCM_SDHOST);
  SDHostInit(SDHOST_BASE);
  //Define the Clock Speed for the Card
  SDHostSetExpClk(SDHOST_BASE, PRCMPeripheralClockGet(PRCM_SDHOST), 15000000);

  typedef struct
{
unsigned long ulCardType;
unsigned long long ullCapacity;
unsigned long ulVersion;
unsigned long ulCapClass;
unsigned short ulRCA;
}CardAttrib_t;
  
  //You can do the Card Detection and Initialization Here
  CardInit(CardAttrib_t *CardAttrib){
      unsigned long ulRet;
      unsigned long ulResp[4];
      //
      // Initialize the attributes.
      //
      CardAttrib->ulCardType = CARD_TYPE_UNKNOWN;
      CardAttrib->ulCapClass = CARD_CAP_CLASS_SDSC;
      CardAttrib->ulRCA = 0;
      CardAttrib->ulVersion = CARD_VERSION_1;
      //
      // Send std GO IDLE command
      //
      if( SendCmd(CMD_GO_IDLE_STATE, 0) == 0)
        {
        ulRet = SendCmd(CMD_SEND_IF_COND,0x00000100);
        //
        // It's a SD ver 2.0 or higher card
        //
        if(ulRet == 0)
        {
        CardAttrib->ulVersion = CARD_VERSION_2;
        CardAttrib->ulCardType = CARD_TYPE_SDCARD;
        //
        // Wait for card to become ready.
        //
        do
        {
        //
        // Send ACMD41
        //
        SendCmd(CMD_APP_CMD,0);
        ulRet = SendCmd(CMD_SD_SEND_OP_COND,0x40E00000);
        //
        // Response contains 32-bit OCR register
        //
        SDHostRespGet(SDHOST_BASE,ulResp);
    }while(((ulResp[0] >> 31) == 0));
    if(ulResp[0] &amp; (1UL<<30)) {
          CardAttrib->ulCapClass = CARD_CAP_CLASS_SDHC;
        }
    }
    else //It's a MMC or SD 1.x card
    {
    //
    // Wait for card to become ready.
    //
    do
      {
    if( (ulRet = SendCmd(CMD_APP_CMD,0)) == 0 )
      {
      ulRet = SendCmd(CMD_SD_SEND_OP_COND,0x00E00000);
    //
    // Response contains 32-bit OCR register
    //
    SDHostRespGet(SDHOST_BASE,ulResp);
    }
  }while((ulRet == 0)
    &amp;&amp;((ulResp[0] >>31) == 0));
    //
    // Check the response
    //
    if(ulRet == 0)
    {
    CardAttrib->ulCardType = CARD_TYPE_SDCARD;
    }
      else // CMD 55 is not recognised by SDHost cards.
        {
          //
          // Confirm if its a SDHost card
          //
          ulRet = SendCmd(CMD_SEND_OP_COND,0);
          if( ulRet == 0)
          {
            CardAttrib->ulCardType = CARD_TYPE_MMC;
          }
        }
      }
    }
    //
    // Get the RCA of the attached card
    //
    if(ulRet == 0)
    {
      ulRet = SendCmd(CMD_ALL_SEND_CID,0);
      if( ulRet == 0)
      {
      SendCmd(CMD_SEND_REL_ADDR,0);
      SDHostRespGet(SDHOST_BASE,ulResp);
      //
      // Fill in the RCA
      //
      CardAttrib->ulRCA = (ulResp[0]>> 16);
      //
      // Get tha card capacity
      //
      CardAttrib->ullCapacity = CardCapacityGet(CardAttrib->ulRCA);
      }
    }
    //
    // return status.
    //
    return ulRet;
    }
}

void loop() {
  //You can Perform the Block Read and Write in the loop Reference is in swru367c Page 322

  //Stop the loop from running and let it sit here forever with the while statement
  while(1); 
}




