#ifndef MOVEMENT_RECOGNITION_H
#define MOVEMENT_RECOGNITION_H

#include <math.h>

#define aTHRESHOLD 0.8
#define gxyTHRESHOLD 20
#define gzTHRESHOLD 80
// Function to check for jerks in sensor data
int detectLiftUp(double ax, double ay, double az, double gx, double azthreshold, double gxthreshold) {

    if ((az > azthreshold && az < 0.0) && (fabs(ax)) < aTHRESHOLD && (fabs(ay)) < aTHRESHOLD && fabs(gx) < gxthreshold) {
        System_printf("lift detected\n");
        System_flush();
        return 1; // Lift detected
    }
    return 0; // Lift not detected
}

int detectJerk(double ax, double ay, double az, double gx, double gy, double gz, double axthreshold, double azthreshold) {

    if (ax > axthreshold && az < azthreshold && fabs(gx) < gxyTHRESHOLD && fabs(gy) < gxyTHRESHOLD && fabs(gz) < gzTHRESHOLD) {
        System_printf("jerk detected\n");
        System_flush();
        return 1; // Jerk detected
    }
    return 0; // Jerk not detected
}

int OnTheBack(double ax, double ay, double az, double gx, double azthreshold) {
    if ((az > azthreshold && az > 0.0 ) && fabs(ax) < aTHRESHOLD && fabs(ay) < aTHRESHOLD) {
        System_printf("on the bak detected\n");
        System_flush();
        return 1; // Chilling on the back :D
    }
    return 0; // No chill :(
}


#endif  // MOVEMENT_RECOGNITION_H
