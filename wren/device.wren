foreign class GamepadButton {
    construct new(i) {}

    foreign static south
    foreign static east
    foreign static west
    foreign static north
    foreign static shoulderLeft
    foreign static shoulderRight
    foreign static select
    foreign static start
    foreign static leftStickPress
    foreign static rightStickPress
    foreign static dPadUp
    foreign static dPadRight
    foreign static dPadDown
    foreign static dPadLeft
}

foreign class GamepadAxis {
    construct new(i) {}

    foreign static leftStickX
    foreign static leftStickY
    foreign static rightStickX
    foreign static rightStickY
    foreign static leftTrigger
    foreign static rightTrigger
}

foreign class MouseButton {
    construct new(i) {}

    foreign static left
    foreign static right
    foreign static middle
}

foreign class Key {
    construct new(i) {}

    foreign static right
    foreign static left
    foreign static down
    foreign static up
    foreign static space
    foreign static escape
    foreign static enter

    foreign static a
    foreign static b
    foreign static c
    foreign static d
    foreign static e
    foreign static f
    foreign static g
    foreign static h
    foreign static i
    foreign static j
    foreign static k
    foreign static l
    foreign static m
    foreign static n
    foreign static o
    foreign static p
    foreign static q
    foreign static r
    foreign static s
    foreign static t
    foreign static u
    foreign static v
    foreign static w
    foreign static x
    foreign static y
    foreign static z

    // TODO: add more keys
}

class Input {
    foreign static getAxis(axis)
    foreign static getButton(button)
    foreign static getButtonOnce(button)
    
    foreign static getKey(key)
    foreign static getKeyOnce(key)
    
    foreign static getMouse()
    foreign static getMouseButton(button)
    foreign static getMouseButtonOnce(button)
    foreign static getMouseX()
    foreign static getMouseY()
    
    foreign static getNumTouches()
    foreign static getTouchId(index)
    foreign static getTouchX(index)
    foreign static getTouchY(index)
    
    foreign static setPadVibration(leftRumble, rightRumble)
    foreign static setPadLightbarColor(r, g, b)
    foreign static resetPadLightbarColor()
}

class Screen {
    foreign static width
    foreign static height
}