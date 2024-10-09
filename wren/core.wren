import "meta" for Meta

class Time {
    foreign static time
    foreign static deltaTime
}

foreign class DataTarget {
    construct new(i) {}
    
    foreign static none
    foreign static system
    foreign static debug
    foreign static game
    foreign static player
}

class Data {
    static getBool(name, value) { getBool(name, value, DataTarget.game) }
    static getInt(name, value) { getInt(name, value, DataTarget.game) }
    static getUInt(name, value) { getUInt(name, value, DataTarget.game) }
    static getFloat(name, value) { getFloat(name, value, DataTarget.game) }
    static getDouble(name, value) { getDouble(name, value, DataTarget.game) }
    static getString(name, value) { getString(name, value, DataTarget.game) }

    static setBool(name, value) { setBool(name, value, DataTarget.game) }
    static setInt(name, value) { setInt(name, value, DataTarget.game) }
    static setUInt(name, value) { setUInt(name, value, DataTarget.game) }
    static setFloat(name, value) { setFloat(name, value, DataTarget.game) }
    static setDouble(name, value) { setDouble(name, value, DataTarget.game) }
    static setString(name, value) { setString(name, value, DataTarget.game) }
    
    foreign static getBool(name, value, target)
    foreign static getInt(name, value, target)
    foreign static getUInt(name, value, target)
    foreign static getFloat(name, value, target)
    foreign static getDouble(name, value, target)
    foreign static getString(name, value, target)

    foreign static setBool(name, value, target)
    foreign static setInt(name, value, target)
    foreign static setUInt(name, value, target)
    foreign static setFloat(name, value, target)
    foreign static setDouble(name, value, target)
    foreign static setString(name, value, target)
}

class File {
    foreign static readTextFile(filename)
    foreign static writeTextFile(filename, text)
    foreign static exists(filename)
}

// TODO: Move these

class Device {
    static PlatformPC      { 0 }
    static PlatformPS5     { 1 }
    static PlatformSwitch  { 2 }

    //foreign static getPlatform()
    //foreign static canClose()
    //foreign static requestClose()
}

foreign class Timer {
    construct new() {}
    
    foreign start()
    foreign elapsed()
}

//foreign class Resource {
//    foreign static create(type, filename)
//
//    foreign getData(type)
//    foreign handle
//    foreign isValid
//    foreign isLoaded
//
//    foreign load(filename)
//    //foreign loadData(data)
//    foreign save(filename)
//    foreign unload()    
//}