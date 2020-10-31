package org.openbot.drivemode

import org.openbot.ControlSignal

class Searching : DriveMode {

    var lastTurn = 1

    private var leftControl = 0f
    private var rightControl = 0f
    private var skippedFrames = 0

    override fun getControl(): ControlSignal {
        skippedFrames++
        if (skippedFrames > 8) {
            skippedFrames = 0
            leftControl = 0.6f * lastTurn
            rightControl = -leftControl
        } else  {
            leftControl = 0.0f
            rightControl = 0.0f
        }
        return ControlSignal(leftControl, rightControl)
    }

}