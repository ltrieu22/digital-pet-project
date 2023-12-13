/* Creators */
// Roni Karppinen, Luan Trieu, Valtteri Jokisaari

#ifndef MUSICH
#define MUSICH
#include "buzzer.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

#include <xdc/std.h>
#include <xdc/runtime/System.h>

//States for sounds
enum sounds {OFF=0,GRIDDY};
enum sounds musicState=OFF;

static PIN_Handle hPin;

Void musicTask(UArg arg0, UArg arg1) {
    while (1) {
        switch (musicState) {

        case OFF:
            break;

        case GRIDDY: //I said right foot creep, ooh, I'm walking with that heater
            // Our favourite song composed as a sensortag version
            buzzerOpen(hPin);

            buzzerSetFrequency(246.94);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(392.00);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(440.00);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(246.94);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(392.00);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(440.00);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(246.94);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(392.00);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(440.00);
            Task_sleep(250000 / Clock_tickPeriod);

            buzzerSetFrequency(0);
            buzzerClose();
            musicState = OFF;
            break;
          default:
              break;
          }
      Task_sleep(200000L / Clock_tickPeriod);
   }
}


#endif
