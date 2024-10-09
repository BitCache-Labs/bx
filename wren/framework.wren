import "ecs" for Entity

// Game object framework

foreign class GameObjectData {
    construct new() {}
    foreign type
    foreign static create(type)
    foreign static load(filepath)
}

foreign class GameObjectBase {
    construct new(data, obj) {}
    foreign initialize(data)
    foreign destroy()
    foreign entity
    foreign name
    foreign name=(v)
}

class GameObject {
    construct new(data) {
        _base = GameObjectBase.new(data, this)
    }
    initialize(data) { _base.initialize(data) }
    destroy() { _base.destroy() }

    // Bindings
    name { _base.name }
    name=(v) { _base.name = v }

    isValid { _base.entity.isValid }
    hasComponent(cmp) { _base.entity.hasComponent(cmp) }
    addComponent(cmp) { _base.entity.addComponent(cmp) }
    getComponent(cmp) { _base.entity.getComponent(cmp) }
    removeComponent(cmp) { _base.entity.removeComponent(cmp) }

    // Entry points for game logic
    start() {}
    update() {}

    // Public static functions
    foreign static register(type)

    static create(type) {
        var data = GameObjectData.create(type)
        var gameObj = type.new(data)
        gameObj.initialize(data)
        return gameObj
    }

    static load(filepath) {
        var data = GameObjectData.load(filepath)
        var gameObj = data.type.new(data)
        gameObj.initialize(data)
        return gameObj
    }
}

foreign class Scene {
    construct new() {}
}

// Components

foreign class Animator {
    construct new() {}

    foreign hasEnded
    foreign duration(v)

    foreign current
    foreign current=(v)

    foreign speed
    foreign speed=(v)

    foreign looping
    foreign looping=(v)

    foreign getBoneMatrix(v)

    toString {}
}

foreign class Attributes {
    construct new() {}
    toString {}
}

foreign class AudioListener {
    construct new() {}
    toString {}
}

foreign class AudioSource {
    construct new() {}
    toString {}
}

foreign class Camera {
    construct new() {}

    foreign fov
    foreign fov=(v)

    foreign aspect
    foreign aspect=(v)

    foreign near
    foreign near=(v)

    foreign far
    foreign far=(v)

    toString { "[%(fov), %(aspect), %(near), %(far)]" }
}

foreign class CharacterController {
    construct new() {}

    foreign offset
    foreign offset=(v)

    foreign width
    foreign width=(v)

    foreign height
    foreign height=(v)

    foreign moveVector=(v)

    foreign linearVelocity=(v)
    //foreign setAngularVelocity(v)

    foreign rotation=(v)
    foreign applyImpulse (v)

    toString {}
}

foreign class Collider {
    construct new() {}
    toString {}
}

foreign class Light {
    construct new() {}

    foreign intensity
    foreign intensity=(v)
    
    foreign constant
    foreign constant=(v)

    foreign linear
    foreign linear=(v)

    foreign quadratic
    foreign quadratic=(v)

    foreign color
    foreign color=(v)

    toString {}
}

foreign class MeshFilter {
    construct new() {}

    //foreign mesh
    //foreign mesh=(v)

    toString {}//{ "%(name)" }
}

foreign class MeshRenderer {
    construct new() {}
    
    //foreign material
    //foreign material=(v)

    toString {}
}

foreign class RigidBody {
    construct new() {}
    toString {}
}

foreign class Spline {
    construct new() {}
    toString {}
}

foreign class Transform {
    construct new() {}

    foreign position
    foreign position=(v)

    foreign rotation
    foreign rotation=(v)

    foreign scale
    foreign scale=(v)

    foreign matrix
    foreign matrix=(v)

    toString {}
}