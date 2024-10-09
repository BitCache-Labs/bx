foreign class ColliderShape {
    construct new(i) {}
    
    foreign static plane
    foreign static box
    foreign static sphere
    foreign static capsule
    foreign static mesh
}

foreign class ColliderAxis {
    construct new(i) {}
    
    foreign static axisX
    foreign static axisY
    foreign static axisZ
}

foreign class ColliderFlags {
    construct new(i) {}
    
    foreign static dynamic
    foreign static Static
    foreign static kinematic
    foreign static character
}

foreign class CollisionFlags {
    construct new(i) {}

    foreign |(v)
    foreign &(v)
    foreign ^(v)
    
    foreign static default
    foreign static Static
    foreign static kinematic
    foreign static debris
    foreign static trigger
    foreign static character
    foreign static all
}

foreign class CastHitResult {
	construct new() {}

    foreign hasHit
	foreign point
	foreign normal

	foreign gameObject
}

class Physics {
	foreign static rayCast(origin, direction, distance, group, mask)
	foreign static rayCastAll(origin, direction, distance, group, mask)
	foreign static sphereCast(origin, direction, distance, radius, group, mask)
	foreign static sphereCastAll(origin, direction, distance, radius, group, mask)
}