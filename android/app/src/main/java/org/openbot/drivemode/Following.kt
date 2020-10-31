package org.openbot.drivemode

import org.openbot.ControlSignal

class Following : DriveMode {

    var XPosition: Float? = null;

    private var leftControl = 0f
    private var rightControl = 0f

    private var searching = Searching()
    private var skippedFrames = 0

    override fun getControl(): ControlSignal {
        if (XPosition != null) {
            skippedFrames = 0
            // TODO test this
            XPosition = XPosition!! / 3
            if (XPosition!! < 0) {
                searching.lastTurn = -1
                leftControl = 1.0f
                rightControl = 1.0f + XPosition!!
            } else {
                searching.lastTurn = 1
                leftControl = 1.0f - XPosition!!
                rightControl = 1.0f
            }
        } else {
            skippedFrames++
            if (skippedFrames > 3) {
                return searching.getControl()
            } else {
                leftControl = 0.5f
                rightControl = 0.5f
            }
        }
        return ControlSignal(leftControl, rightControl)
    }
}
