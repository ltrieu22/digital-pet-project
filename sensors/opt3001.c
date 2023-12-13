/*
 * opt3001.c
 *
 *  Created on: 22.7.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/opt3001.pdf
 */

#include <string.h>
#include <math.h>
#include <stdio.h>

#include <xdc/runtime/System.h>

#include "sensors/opt3001.h"
#include "Board.h"

void opt3001_setup(I2C_Handle *i2c) {

	I2C_Transaction i2cTransaction;
	char itxBuffer[3];

    i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
    itxBuffer[0] = OPT3001_REG_CONFIG;
    itxBuffer[1] = 0xCE; // continuous mode s.22
    itxBuffer[2] = 0x02;
    i2cTransaction.writeBuf = itxBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (I2C_transfer(*i2c, &i2cTransaction)) {

        System_printf("OPT3001: Config write ok\n");
    } else {
        System_printf("OPT3001: Config write failed!\n");
    }
    System_flush();

}

uint16_t opt3001_get_status(I2C_Handle *i2c) {

	uint16_t e=0;
	I2C_Transaction i2cTransaction;
	char itxBuffer[1];
	char irxBuffer[2];

	/* Read sensor state */
	i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
	itxBuffer[0] = OPT3001_REG_CONFIG;
	i2cTransaction.writeBuf = itxBuffer;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = irxBuffer;
	i2cTransaction.readCount = 2;

	if (I2C_transfer(*i2c, &i2cTransaction)) {

		e = (irxBuffer[0] << 8) | irxBuffer[1];
	} else {

		e = 0;
	}
	return e;
}

/**************** JTKJ: DO NOT MODIFY ANYTHING ABOVE THIS LINE ****************/

double opt3001_get_data(I2C_Handle *i2c) {
	
	double lux = -1.0; // return value of the function
    // Buffer sizes with this sensor
    uint8_t txBuffer[1];
    uint8_t rxBuffer[2];

	// I2C message data structure 
    I2C_Transaction i2cTransaction1;
    i2cTransaction1.slaveAddress = Board_OPT3001_ADDR;
    txBuffer[0] = OPT3001_REG_RESULT;
    i2cTransaction1.writeBuf = txBuffer;
    i2cTransaction1.writeCount = 1;
    i2cTransaction1.readBuf = rxBuffer;
    i2cTransaction1.readCount = 2;
	if (opt3001_get_status(i2c) & OPT3001_DATA_READY) {
		if (I2C_transfer(*i2c, &i2cTransaction1)) {
			// Extract data from received buffer
		    uint16_t rekisteri = rxBuffer[0]<<8;
		    rekisteri = rekisteri | rxBuffer[1];
		    uint8_t E = (rekisteri>> 12) & 0x0F;
		    uint16_t R = rekisteri & 0x0FFF;
			//  Calculate lux with given formula
		    lux = 0.01 * pow(2, E) * R;
		} else {
			System_printf("OPT3001: Data read failed!\n");
			System_flush();
		}
	}

	return lux;
}
