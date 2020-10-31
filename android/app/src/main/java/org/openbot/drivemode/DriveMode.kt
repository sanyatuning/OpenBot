package org.openbot.drivemode

import org.openbot.ControlSignal

interface DriveMode {

    public fun getControl(): ControlSignal

}
