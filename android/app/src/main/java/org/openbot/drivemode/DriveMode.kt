package org.openbot.drivemode

import org.openbot.env.Vehicle

interface DriveMode {

    public fun getControl(sensorOrientation: Int): Vehicle.Control

}
