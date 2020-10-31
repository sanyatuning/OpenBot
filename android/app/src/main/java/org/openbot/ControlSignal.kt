package org.openbot

class ControlSignal(left: Float, right: Float) {
    val left: Float
    val right: Float

    init {
        this.left = Math.max(-1f, Math.min(1f, left))
        this.right = Math.max(-1f, Math.min(1f, right))
    }
}
