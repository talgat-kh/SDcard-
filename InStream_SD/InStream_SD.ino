#include "../system/driverlib/sdhost.h"
#include "../system/driverlib/timer.h"
#include "../system/driverlib/prcm.h"

#define CMD_GO_IDLE_STATE    SDHOST_CMD_0
#define CMD_SEND_IF_COND     SDHOST_CMD_8|SDHOST_RESP_LEN_48
#define CMD_SEND_CSD         SDHOST_CMD_9|SDHOST_RESP_LEN_136
#define CMD_APP_CMD          SDHOST_CMD_55|SDHOST_RESP_LEN_48
#define CMD_SD_SEND_OP_COND  SDHOST_CMD_41|SDHOST_RESP_LEN_48
#define CMD_SEND_OP_COND     SDHOST_CMD_1|SDHOST_RESP_LEN_48
#define CMD_READ_SINGLE_BLK  SDHOST_CMD_17|SDHOST_RD_CMD|SDHOST_RESP_LEN_48
#define CMD_READ_MULTI_BLK   SDHOST_CMD_18|SDHOST_RD_CMD|SDHOST_RESP_LEN_48|SDHOST_MULTI_BLK
#define CMD_WRITE_SINGLE_BLK SDHOST_CMD_24|SDHOST_WR_CMD|SDHOST_RESP_LEN_48
#define CMD_WRITE_MULTI_BLK  SDHOST_CMD_25|SDHOST_WR_CMD|SDHOST_RESP_LEN_48|SDHOST_MULTI_BLK
#define CMD_SELECT_CARD      SDHOST_CMD_7|SDHOST_RESP_LEN_48B
#define CMD_DESELECT_CARD    SDHOST_CMD_7
#define CMD_STOP_TRANS       SDHOST_CMD_12|SDHOST_RESP_LEN_48B
#define CMD_SET_BLK_CNT      SDHOST_CMD_23|SDHOST_RESP_LEN_48
#define CMD_ALL_SEND_CID     SDHOST_CMD_2|SDHOST_RESP_LEN_136
#define CMD_SEND_REL_ADDR    SDHOST_CMD_3|SDHOST_RESP_LEN_48

#define RETRY_TIMEOUT        2000

#define CARD_TYPE_UNKNOWN    0
#define CARD_TYPE_MMC        1
#define CARD_TYPE_SDCARD     2

#define CARD_CAP_CLASS_SDSC  0
#define CARD_CAP_CLASS_SDHC  1

#define CARD_VERSION_1       0
#define CARD_VERSION_2       1

typedef struct
{
  unsigned long  ulCardType;
  unsigned long  long ullCapacity;
  unsigned long  ulVersion;
  unsigned long  ulCapClass;
  unsigned short ulRCA;
}CardAttrib_t;


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
  MAP_PinConfigSet(PIN_02,PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);
  //SDCard_DATA0 Pin Define
  MAP_PinTypeSDHost(PIN_64, PIN_MODE_6);
  MAP_PinConfigSet(PIN_64,PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

  //Set the clock pin as an output
  PinDirModeSet(PIN_01, PIN_DIR_MODE_OUT);

  //Soft Reset and initialize the SD Host Controller
  //Talgat I am unsure if the SDHOST_BASE register is correct.  There may be an additional character to describe which BASE is linked to our pins
  PRCMPeripheralReset(PRCM_SDHOST);
  SDHostInit(SDHOST_BASE);
  //Define the Clock Speed for the Card
  SDHostSetExpClk(SDHOST_BASE, PRCMPeripheralClockGet(PRCM_SDHOST), 15000000);

  
}

void loop() {
  //You can Perform the Block Read and Write in the loop Reference is in swru367c Page 322

  //Stop the loop from running and let it sit here forever with the while statement
  while(1); 
}



//Define Functions You can Use to Explore Card 
//Taken from the sdhost example in the TI SDK 

static unsigned long
SendCmd(unsigned long ulCmd, unsigned long ulArg)
{
    unsigned long ulStatus;

    //
    // Clear interrupt status
    //
    MAP_SDHostIntClear(SDHOST_BASE,0xFFFFFFFF);

    //
    // Send command
    //
    MAP_SDHostCmdSend(SDHOST_BASE,ulCmd,ulArg);

    //
    // Wait for command complete or error
    //
    do
    {
        ulStatus = MAP_SDHostIntStatus(SDHOST_BASE);
        ulStatus = (ulStatus & (SDHOST_INT_CC|SDHOST_INT_ERRI));
    }
    while( !ulStatus );

    //
    // Check error status
    //
    if(ulStatus & SDHOST_INT_ERRI)
    {
        //
        // Reset the command line
        //
        MAP_SDHostCmdReset(SDHOST_BASE);
        return 1;
    }
    else
    {
        return 0;
    }
}

static unsigned long
CardInit(CardAttrib_t *CardAttrib)
{
    unsigned long ulRet;
    unsigned long ulResp[4];

    //
    // Initialize the attributes.
    //
    CardAttrib->ulCardType = CARD_TYPE_UNKNOWN;
    CardAttrib->ulCapClass = CARD_CAP_CLASS_SDSC;
    CardAttrib->ulRCA      = 0;
    CardAttrib->ulVersion  = CARD_VERSION_1;

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
                MAP_SDHostRespGet(SDHOST_BASE,ulResp);

            }while(((ulResp[0] >> 31) == 0));

            if(ulResp[0] & (1UL<<30))
            {
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
                    MAP_SDHostRespGet(SDHOST_BASE,ulResp);
                }
            }while((ulRet == 0) && ((ulResp[0] >> 31) == 0));

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
            MAP_SDHostRespGet(SDHOST_BASE,ulResp);

            //
            //  Fill in the RCA
            //
            CardAttrib->ulRCA = (ulResp[0] >> 16);

            //
            // Get tha card capacity
            //
            //CardAttrib->ullCapacity = CardCapacityGet(CardAttrib -> ulRCA);
        }
    }

    //
    // return status.
    //
    return ulRet;
}

static unsigned long long
CardCapacityGet(unsigned short ulRCA)
{
    unsigned long ulRet;
    unsigned long ulResp[4];
    unsigned long long ullCapacity;
    unsigned long ulBlockSize;
    unsigned long ulBlockCount;
    unsigned long ulCSizeMult;
    unsigned long ulCSize;

    //
    // Read the CSD register
    //
    ulRet = SendCmd(CMD_SEND_CSD,(ulRCA << 16));
    if(ulRet == 0)
    {
        //
        // Read the response
        //
        MAP_SDHostRespGet(SDHOST_BASE,ulResp);
    }
    else
    {
        return 0;
    }

    //
    // 136 bit CSD register is read into an array of 4 words.
    // ulResp[0] = CSD[31:0]
    // ulResp[1] = CSD[63:32]
    // ulResp[2] = CSD[95:64]
    // ulResp[3] = CSD[127:96]
    //
    if( ((ulResp[3] >> 30) & 0x1) == 1)
    {
        ulBlockSize = 512 * 1024;
        ulBlockCount = (ulResp[1] >> 16 | ((ulResp[2] & 0x3F) << 16)) + 1;
    }
    else
    {
        ulBlockSize  = 1 << ((ulResp[2] >> 16) & 0xF);
        ulCSizeMult  = ((ulResp[1] >> 15) & 0x7);
        ulCSize      = ((ulResp[1] >> 30) | (ulResp[2] & 0x3FF) << 2);
        ulBlockCount = (ulCSize + 1) * (1<<(ulCSizeMult + 2));
    }

    //
    // Calculate the card capacity in bytes
    //
    ullCapacity = (unsigned long long)ulBlockSize * (ulBlockCount );

    //
    // Return the capacity
    //
    return ullCapacity;
}

