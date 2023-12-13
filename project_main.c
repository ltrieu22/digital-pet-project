/* Creators */
// Roni Karppinen, Luan Trieu, Valtteri Jokisaari

/* C Standard library */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"
#include "buzzer.h"

/* MPU Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

#define STACKSIZE 2048

/* Own files */
#include "movement_recognition.h"
#include "messages.h"
#include "music.h"

/* Task */
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];
Char musicTaskStack[STACKSIZE];
Char taskStack[STACKSIZE];

//State machine for data
enum state { WAITING=1, DATA_READY, SEND};
enum state programState = WAITING;
//State machine for action
enum actionState { CHILL=1, EXERCISE, FEED, PET, ACTIVATE};
enum actionState actionStatus = CHILL;

// Uart sending / receiving buffers
char uartBuffer[80];
char uartStr[80];

// Global variables
double ambientLight = -1000.0;
float ax;
float ay;
float az;
float gx;
float gy;
float gz;

// Reference strings for message validity
char idStr[] = "3253";
char warningStr[] = "BEEP";

// Initialize variables for sensorTask
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;

//Prototypes
void sendMessage(UART_Handle handle, char *payload);
void uartFxn(UART_Handle handle, void *rxBuf, size_t len);
void uartTaskFxn(UArg arg0, UArg arg1);
int checkSensorData(float ax, float ay, float az, float gx, float gy, float gz);
void sensorTaskFxn(UArg arg0, UArg arg1);

PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE  // Asetustaulukko lopetetaan aina t채ll채 vakiolla
};

PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina t채ll채 vakiolla
};

void buttonFxn(PIN_Handle ledHandle, PIN_Id pinId) {
    uint_t pinValue = PIN_getOutputValue( Board_LED0 );
       pinValue = !pinValue;
       PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
}

void sendMessage(UART_Handle handle, char *payload){
    //Send message through UART when it is plugged with USB
    UART_write(handle, payload, strlen(payload)+1);
}

void uartFxn(UART_Handle handle, void *rxBuf, size_t len){
    //Reading UART
    UART_read(handle, rxBuf, 80);
    // Check if the message contains the specified ID and warning and play a sound if the conditions are met
    if (strstr(rxBuf, idStr) != NULL && strstr(rxBuf, warningStr) != NULL) {
       musicState = GRIDDY;
    }
}

/* Task Functions */
void uartTaskFxn(UArg arg0, UArg arg1) {
    UART_Handle uart;
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readMode = UART_MODE_CALLBACK; // Interrupt based response luento8
    uartParams.readCallback = &uartFxn; // callback function handler / ISR
    uartParams.baudRate = 9600; // SPEED 9600baud
    uartParams.dataLength = UART_LEN_8; // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE; // 1
    //Opening UART
    uart = UART_open(Board_UART, &uartParams);
    if (uart == NULL) {
      System_abort("Error opening the UART");
    }
    //Reading incoming messages through UART
    UART_read(uart, uartBuffer, 80);

    while (1) {
        // If the programState is send and the actionStatus is different than CHILL, device sends designated messages according to actionStatus
        if (programState == SEND) {
            switch (actionStatus) {
                case EXERCISE:
                    sendMessage(uart, EXERCISEMESSAGE);
                    break;
                case FEED:
                    sendMessage(uart, FEEDMESSAGE);
                    break;
                case PET:
                    sendMessage(uart, PETMESSAGE);
                    break;
                case ACTIVATE:
                    sendMessage(uart, ACTIVATEMESSAGE);
                    break;
                default:
                    break;
            }
            programState = WAITING;
        }
        // Resetting the buffer
        memset(uartStr,0,80);
        // Once per second, you can modify this
        Task_sleep(150000 / Clock_tickPeriod);
    }
}


int checkSensorData(float ax, float ay, float az, float gx, float gy, float gz) {

    double azThreshold = -0.8;
    double axThreshold = 0.2;
    double gxThreshold = 30;

    int liftDetected = 0;
    int jerkDetected = 0;
    int onTheBackDetected = 0;

    //Setting up functions to variables
    liftDetected = detectLiftUp(ax, ay, az, gx, azThreshold, gxThreshold);
    jerkDetected = detectJerk(ax, ay, az, gx, gy, gz, axThreshold, azThreshold);
    onTheBackDetected = OnTheBack(ax, ay, az, gx, azThreshold);

    if (liftDetected || jerkDetected) { // Setting actionStatus to EXERCISE if conditions are met
        actionStatus = EXERCISE;
    } else if ((onTheBackDetected == 1 && actionStatus != PET && ambientLight < 1 && ambientLight > 0)) { // Setting actionStatus to PET if onTheBack
        actionStatus = PET;
    } else if (onTheBackDetected == 1 && ambientLight > 10){  // Setting actionStatus to FEED if onTheBack and bright surroundings
        actionStatus = FEED;
    } else {
        //If not any action has been performed, keep the actionStatus as a CHILL and return 0
        actionStatus = CHILL;
        return 0;
    }
    // return 1 if actionStatus is changed
    return 1;
}

void sensorTaskFxn(UArg arg0, UArg arg1) {

    I2C_Handle      i2c;
    I2C_Params      i2cParams;

    // MPU setup + calibration
    I2C_Handle      i2cMPU;
    I2C_Params      i2cMPUParams;

    // MPU power pin global variables
    static PIN_Handle hMpuPin;
    static PIN_State  MpuPinState;

    // MPU power pin
    static PIN_Config MpuPinConfig[] = {
        Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
        PIN_TERMINATE
    };

    // MPU uses its own I2C interface
    static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
        .pinSDA = Board_I2C0_SDA1,
        .pinSCL = Board_I2C0_SCL1
    };

    // Open MPU power pin
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
        System_abort("Pin open failed!");
    }

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    // Note the different configuration below
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // MPU power on
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // Wait 100ms for the MPU sensor to power up
    Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    // MPU open i2c
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }
    // MPU setup and calibration
    System_printf("MPU9250: Setup and calibration...\n");
    System_flush();

    mpu9250_setup(&i2cMPU);

    System_printf("MPU9250: Setup and calibration OK\n");
    System_flush();
    I2C_close(i2cMPU);

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    // Before calling the setup function, insert 100ms delay with Task_sleep
    Task_sleep(100000 / Clock_tickPeriod);
    opt3001_setup(&i2c);
    I2C_close(i2c);
    while (1) {
        if(programState == WAITING) {
            // Opening I2C, reading ambientLight and then closing the I2C
            i2c = I2C_open(Board_I2C_TMP, &i2cParams);
            ambientLight = opt3001_get_data(&i2c);
            I2C_close(i2c);
            //Opening I2C and reading data from MPU9250 sensor and then closing the I2C
            i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
            I2C_close(i2cMPU);
            // Check data
            if (checkSensorData(ax, ay, az, gx, gy, gz) == 1) {
                programState = SEND;
            }
            // Resetting the buffer
            memset(uartStr,0,80);
        }
        Task_sleep(200000 / Clock_tickPeriod);
    }
}


Int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;
    Task_Handle musicTaskHandle;
    Task_Params musicTaskParams;

    // Initialize board
    Board_initGeneral();

    // Initialize i2c bus
    // Initialize UART
    Board_initUART();
    Board_initI2C();

    // Open the button and led pins
    buttonHandle = PIN_open(&buttonState, buttonConfig);
    if(!buttonHandle) {
        System_abort("Error initializing button pins\n");
    }
    ledHandle = PIN_open(&ledState, ledConfig);
    if(!ledHandle) {
        System_abort("Error initializing LED pins\n");
    }
    if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
        System_abort("Error registering button callback function");
    }

    /* Sensor task */
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Sensor task create failed!");
    }

    /* UART task */
    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority = 1;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("UART task create failed!");
    }

    /* Music task */
    Task_Params_init(&musicTaskParams);
    musicTaskParams.stackSize = STACKSIZE;
    musicTaskParams.stack = &musicTaskStack;
    musicTaskParams.priority = 2;
    musicTaskHandle = Task_create(musicTask, &musicTaskParams, NULL);
    if (musicTaskHandle == NULL) {
        System_abort("Task create failed");
    }

    /* Sanity check */
    System_printf("Hello world(and Ivan)!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
