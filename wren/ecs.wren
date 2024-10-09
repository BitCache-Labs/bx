foreign class Entity {
    construct new(id) { }

    foreign static create()

    foreign static invalid
    
    foreign isValid
    //foreign getComponentMask()
    //foreign hasComponents()
    foreign hasComponent(cmp)
    foreign addComponent(cmp)
    foreign getComponent(cmp)
    foreign removeComponent(cmp)
    foreign destroy()
}