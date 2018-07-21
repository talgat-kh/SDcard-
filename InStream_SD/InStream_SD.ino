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




