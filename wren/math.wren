import "random" for Random

class Math {
    static pi { 3.14159265359 }
    static lerp(a, b, t) { (a * (1.0 - t)) + (b * t) }
    static damp(a, b, lambda, dt) { lerp(a, b, 1.0 - (-lambda * dt).exp) }    
    static min(l, r) { l < r ? l : r }
    static max(l, r) { l > r ? l : r }

    static invLerp(a, b, v) {
	    var  t = (v - a) / (b - a)
	    t = max(0.0, min(t, 1.0))
	    return t
    }

    static remap(iF, iT, oF, oT, v) {
	    var t = invLerp(iF, iT, v)
	    return lerp(oF, oT, t)
    }

    static radians(deg) { deg / 180.0 * 3.14159265359 }
    static degrees(rad) { rad * 180.0 / 3.14159265359 }
    static mod(x, m)    { (x % m + m) % m }   
    static clamp(a, f, t) { max(min(a, t), f) }
    /*static slerp(a,  b,  t) {
	    var CS = (1 - t) * (a.cos) + t * (b.cos)
	    var SN = (1 - t) * (a.sin) + t * (b.sin)
	    return Vec2.new(CS, SN).atan2
    }
    static sdamp(a, b, lambda, dt) { slerp(a, b, 1.0 - (-lambda * dt).exp) }    
    */
}

/*class Bits {
    static switchOnBitFlag(flags, bit) { flags | bit }
    static switchOffBitFlag(flags, bit) { flags & (~bit) }
    static checkBitFlag(flags, bit) { (flags & bit) == bit }
    static checkBitFlagOverlap(flag0, flag1) { (flag0 & flag1) != 0 }
}

class Color {
    construct new(r, g, b, a) {
        _r = r
        _g = g
        _b = b
        _a = a
    }
    construct new(r, g, b) {
        _r = r
        _g = g
        _b = b
        _a = 255
    }

    a { _a }
    r { _r }
    g { _g }
    b { _b }
    a=(v) { _a = v }
    r=(v) { _r = v }
    g=(v) { _g = v }
    b=(v) { _b = v }

    toNum { r << 24 | g << 16 | b << 8 | a }
    static fromNum(v) {
        var a = v & 0xFF
        var b = (v >> 8) & 0xFF
        var g = (v >> 16) & 0xFF
        var r = (v >> 24) & 0xFF
        return Color.new(r, g, b, a)
    }

    // Based on https://gist.github.com/983/e170a24ae8eba2cd174f
    static fromHsv(h, s, v) {
        var kx = 1.0
        var ky = 2.0 / 3.0
        var kz = 1.0 / 3.0
        var kw = 3.0

        var px = ((h + kx).fraction * 6.0 - kw).abs
        var py = ((h + ky).fraction * 6.0 - kw).abs
        var pz = ((h + kz).fraction * 6.0 - kw).abs

        var rr = v * Math.lerp(kx, Math.clamp(px - kx, 0.0, 1.0), s)
        var rg = v * Math.lerp(kx, Math.clamp(py - kx, 0.0, 1.0), s)
        var rb = v * Math.lerp(kx, Math.clamp(pz - kx, 0.0, 1.0), s)
        
        return Color.new(rr * 255, rg * 255, rb * 255)
    }

    static mix(a, b, t) {
        var cr = Math.lerp(a.r, b.r, t)
        var cg = Math.lerp(a.g, b.g, t)
        var cb = Math.lerp(a.b, b.b, t)
        var ca = Math.lerp(a.a, b.a, t)
        return Color.new(cr, cg, cg, ca)
    }

    toString { "[r:%(_r) g:%(_g) b:%(_b) a:%(_a)]" }
}

class Geom {
    
    // Based on https://stackoverflow.com/questions/1073336/circle-line-segment-collision-detection-algorithm        
    static distanceSegmentToPoint(a, b, c) {
        // Compute vectors AC and AB
        var ac = c - a
        var ab = b - a

        // Get point D by taking the projection of AC onto AB then adding the offset of A
        var d = Vec2.project(ac, ab) + a

        var ad = d - a
        // D might not be on AB so calculate k of D down AB (aka solve AD = k * AB)
        // We can use either component, but choose larger value to reduce the chance of dividing by zero
        var k = ab.x.abs > ab.y.abs ? ad.x / ab.x : ad.y / ab.y

        // Check if D is off either end of the line segment
        if (k <= 0.0) {
            return Vec2.distance(c,a)
        } else if (k >= 1.0) {
            return Vec2.distance(c, b)
        }

        return Vec2.distance(c, d)
    }
}*/

// Representation of 2D vectors and points.
foreign class Vec2 {
    // 	Creates a new vector with given x, y components.
    construct new(x, y) {}

    // X component of the vector.
    foreign x
    foreign x=(v)

    // Y component of the vector.
    foreign y
    foreign y=(v)

    //sqrMagnitude { x * x + y * y }
    //magnitude { sqrMagnitude.sqrt }
    //normalized { this / this.magnitude }

    //[i] {
    //    if (i == 0) {
    //        return _x
    //    } else if (i == 1) {
    //        return _y
    //    }
    //}

    //+(other) { Vec2.new(x + other.x, y + other.y) }
    //-{ Vec2.new(-x, -y)}
    //-(other) { this + -other }
    //*(v) { Vec2.new(x * v, y * v) }
    ///(v) { Vec2.new(x / v, y / v) }
    //==(other) { (other != null) && (x == other.x) && (y == other.y) }
    //!=(other) { !(this == other) }

    static left { Vec2.new(-1, 0) }
    static right { Vec2.new(1, 0) }
    static up { Vec2.new(0, 1) }
    static down { Vec2.new(0, -1) }
    static one { Vec2.new(1, 1) }
    static zero { Vec2.new(0, 0) }
    static positiveInfinity { Vec2.new(Num.infinity, Num.infinity) }
    static negativeInfinity { Vec2.new(-Num.infinity, -Num.infinity) }

    //static dot(a, b) { a.x * b.x + a.y * b.y }
    //static distance(a, b) { (a - b).magnitude }
    //static normalize(v) { Vec2.new(v.x, v.y).normalized }
    //static scale(a, b) { Vec2.new(a.x * b.x, a.y * b.y) }

    //static min(a, b) { Vec2.new(a.x.min(b.x), a.y.min(b.y)) }
    //static max(a, b) { Vec2.new(a.x.max(b.x), a.y.max(b.y)) }

    ////static angle(a, b) { 0 }
    ////static signedAngle(a, b) { 0 }
    ////static clampMagnitude(v, max) { 0 }
    ////static perpendicular(v, d) { Vec2.new() }
    ////static reflect(d, n) { Vec2.new() }
    
    ////static moveTowards(a, b, dt) { Vec2.new() }
    ////static lerpUnclamped(a, b, t) { Vec2.new() }
    ////static lerp(a, b, t) { Vec2.new() }
    ////static smoothDamp(a, b, t) { Vec2.new() }

    toString { "[%(x), %(y)]" }
}

// Representation of 3D vectors and points.
foreign class Vec3 {
    // 	Creates a new vector with given x, y, z components.
    construct new(x, y, z) {}
    
    static copy(v) { Vec3.new(v.x, v.y, v.z) }
    copy { Vec3.new(x, y, z) }

    // X component of the vector.
    foreign x
    foreign x=(v)

    // Y component of the vector.
    foreign y
    foreign y=(v)

    // Z component of the vector.
    foreign z
    foreign z=(v)

    // Returns the squared length of this vector.
    foreign sqrMagnitude

    // Returns the length of this vector.
    foreign magnitude

    // Returns this vector with a magnitude of 1.
    foreign normalized

    // Access the x, y, z components using [0], [1], [2] respectively.
    foreign at(i)
    [i] { at(i) }

    // Set x, y and z components of an existing vector.
    foreign set(x, y, z)

    // Returns a formatted string for this vector.
    toString { "[%(x), %(y), %(z)]" }

    // Adds two vectors.
    foreign +(rhs)

    // Subtracts one vector from another.
    foreign negate
    - { negate }
    foreign -(rhs)

    // Multiplies a vector by a number.
    foreign *(rhs)

    // 	Divides a vector by a number.
    foreign /(rhs)

    // Returns true if two vectors are approximately equal.
    //==(rhs) { (other != null) && (x == other.x) && (y == other.y) && (z == other.z) }
    //!=(rhs) { !(this == other) }

    static left { Vec3.new(-1, 0, 0) }
    static right { Vec3.new(1, 0, 0) }
    static up { Vec3.new(0, 1, 0) }
    static down { Vec3.new(0, -1, 0) }
    static forward { Vec3.new(0, 0, 1) }
    static back { Vec3.new(0, 0, -1) }
    static one { Vec3.new(1, 1, 1) }
    static zero { Vec3.new(0, 0, 0) }
    static positiveInfinity { Vec3.new(Num.infinity, Num.infinity, Num.infinity) }
    static negativeInfinity { Vec3.new(-Num.infinity, -Num.infinity, -Num.infinity) }

    // Dot Product of two vectors.
    foreign static dot(a, b)

    // Returns the distance between a and b.
    //static distance(a, b) { (a - b).magnitude }

    // Makes this vector have a magnitude of 1.
    foreign static normalize(v)

    // Multiplies two vectors component-wise.
    //static scale(a, b) { Vec3.new(a.x * b.x, a.y * b.y, a.z * b.z) }

    // Returns a vector that is made from the smallest components of two vectors.
    //static min(a, b) { Vec3.new(a.x.min(b.x), a.y.min(b.y), a.z.min(b.z)) }

    // Returns a vector that is made from the largest components of two vectors.
    //static max(a, b) { Vec3.new(a.x.max(b.x), a.y.max(b.y), a.z.max(b.z)) }

    // Calculates the angle between vectors a and b.
    //static angle(a, b) { 0 }

    // Calculates the signed angle between vectors a and b in relation to axis v.
    //static signedAngle(a, b, v) { 0 }

    // Returns a copy of vector with its magnitude clamped to max.
    //static clampMagnitude(v, max) { 0 }

    // Cross Product of two vectors.
    foreign static cross(a, b)

    // Makes vectors normalized and orthogonal to each other.
    //static orthoNormalize(a, b) { Vec3.new() }

    // Projects a vector onto another vector.
    //static project(v, n) { Vec3.new() }

    // Projects a vector onto a plane defined by a normal orthogonal to the plane.
    //static projectOnPlane(v, n) { Vec3.new() }

    // Reflects a vector off the plane defined by a normal.
    //static reflect(d, n) { Vec3.new() }

    // Calculate a position between the points specified by current and target, moving no farther than the distance specified by maxDistanceDelta.
    //static moveTowards(a, b, dt) { Vec3.new() }

    // Rotates a vector current towards target.
    //static rotateTowards(a, b, dr, dm) { Vec3.new() }

    // Linearly interpolates between two points.
    //static lerpUnclamped(a, b, t) { Vec3.new() }

    // Linearly interpolates between two points.
    foreign static lerp(a, b, t)

    // Spherically interpolates between two vectors.
    //static slerpUnclamped(a, b, t) { Vec3.new() }

    // Spherically interpolates between two vectors.
    //static slerp(a, b, t) { Vec3.new() }

    // Gradually changes a vector towards a desired goal over time.
    //static smoothDamp(a, b, t) { Vec3.new() }
}

// Representation of four-dimensional vectors.
foreign class Vec4 {
    // Creates a new vector with given x, y, z, w components.
    construct new(x, y, z, w) {}

    // X component of the vector.
    foreign x
    foreign x=(v)

    // Y component of the vector.
    foreign y
    foreign y=(v)

    // Z component of the vector.
    foreign z
    foreign z=(v)

    // W component of the vector.
    foreign w
    foreign w=(v)

    // Returns the squared length of this vector.
    //sqrMagnitude { x * x + y * y + z * z + w * w }

    // Returns the length of this vector.
    //magnitude { sqrMagnitude.sqrt }

    // Returns this vector with a magnitude of 1.
    //normalized { this / this.magnitude }

    // Access the x, y, z, w components using [0], [1], [2], [3] respectively.
    //[i] {
    //    if (i == 0) {
    //        return _x
    //    } else if (i == 1) {
    //        return _y
    //    } else if (i == 2) {
    //        return _z
    //    } else if (i == 3) {
    //        return _w
    //    }
    //}

    // Set x, y, z and w components of an existing Vector4.
    //set(x, y, z, w) {
    //    _x = x
    //    _y = y
    //    _z = z
    //    _w = w
    //}

    // Returns a formatted string for this vector.
    toString { "[%(x), %(y), %(z), %(w)]" }

    // Adds two vectors.
    //+(other) { Vec3.new(x + other.x, y + other.y, z + other.z, w + other.w) }

    // Subtracts one vector from another.
    //-{ Vec3.new(-x, -y, -z, -w)}
    //-(other) { this + -other }

    // Multiplies a vector by a number.
    //*(v) { Vec3.new(x * v, y * v, z * v, w * v) }

    // Divides a vector by a number.
    ///(v) { Vec3.new(x / v, y / v, z / v, w / v) }

    // Returns true if two vectors are approximately equal.
    //==(other) { (other != null) && (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w) }
    //!=(other) { !(this == other) }

    // Shorthand for writing Vec4.new(1, 1, 1, 1).
    static one { Vec4.new(1, 1, 1, 1) }

    // Shorthand for writing Vec4.new(0, 0, 0, 0).
    static zero { Vec4.new(0, 0, 0, 0) }
    
    // Shorthand for writing Vec4.new(Num.infinity, Num.infinity, Num.infinity, Num.infinity).
    static positiveInfinity { Vec4.new(Num.infinity, Num.infinity, Num.infinity, Num.infinity) }

    // Shorthand for writing Vec4.new(-Num.infinity, -Num.infinity, -Num.infinity, -Num.infinity).
    static negativeInfinity { Vec4.new(-Num.infinity, -Num.infinity, -Num.infinity, -Num.infinity) }

    // Dot Product of two vectors.
    //static dot(a, b) { a.x * b.x + a.y * b.y + a.z * b.z }

    // Returns the distance between a and b.
    //static distance(a, b) { (a - b).magnitude }

    // Makes this vector have a magnitude of 1.
    //static normalize(v) { Vec4.new(v.x, v.y, v.z, v.w).normalized }

    // Multiplies two vectors component-wise.
    //static scale(a, b) { Vec4.new(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w) }

    // Returns a vector that is made from the smallest components of two vectors.
    //static min(a, b) { Vec4.new(a.x.min(b.x), a.y.min(b.y), a.z.min(b.z), a.w.min(b.w)) }

    // Returns a vector that is made from the largest components of two vectors.
    //static max(a, b) { Vec4.new(a.x.max(b.x), a.y.max(b.y), a.z.max(b.z), a.w.max(b.w)) }
    
    // Projects a vector onto another vector.
    //static project(a, b) { Vec4.new() }
    
    // Moves a point current towards target.
    //static moveTowards(a, b, dt) { Vec4.new() }

    // Linearly interpolates between two vectors.
    //static lerpUnclamped(a, b, t) { Vec4.new() }

    // Linearly interpolates between two vectors.
    //static lerp(a, b, t) { Vec4.new() }
}

foreign class Vec4i {
    // Creates a new vector with given x, y, z, w components.
    construct new(x, y, z, w) {}

    // X component of the vector.
    foreign x
    foreign x=(v)

    // Y component of the vector.
    foreign y
    foreign y=(v)

    // Z component of the vector.
    foreign z
    foreign z=(v)

    // W component of the vector.
    foreign w
    foreign w=(v)
}

// Quaternions are used to represent rotations.
foreign class Quat {
    // Constructs new Quaternion with given x,y,z,w components.
    construct new(x, y, z, w) {}

    // X component of the Quaternion. Don't modify this directly unless you know quaternions inside out.
    foreign x
    foreign x=(v)

    // Y component of the Quaternion. Don't modify this directly unless you know quaternions inside out.
    foreign y
    foreign y=(v)

    // Z component of the Quaternion. Don't modify this directly unless you know quaternions inside out.
    foreign z
    foreign z=(v)

    // W component of the Quaternion. Do not directly modify quaternions.
    foreign w
    foreign w=(v)

    // Returns or sets the euler angle representation of the rotation.
    foreign eulerAngles

    // Returns this quaternion with a magnitude of 1.
    //normalized { this / this.magnitude }

    // Access the x, y, z, w components using [0], [1], [2], [3] respectively.
    //[i] {
    //    if (i == 0) {
    //        return _x
    //    } else if (i == 1) {
    //        return _y
    //    } else if (i == 2) {
    //        return _z
    //    } else if (i == 3) {
    //        return _w
    //    }
    //}

    // Set x, y, z and w components of an existing Quaternion.
    //set(x, y, z, w) {
    //    _x = x
    //    _y = y
    //    _z = z
    //    _w = w
    //}

    // Creates a rotation which rotates from a to b.
    //setFromToRotation(a, b) {
    //}

    // Creates a rotation with the specified forward and upwards directions.
    //setLookRotation(f, u) {
    //}

    // Converts a rotation to angle(w)-axis(x,y,z) representation (angles in degrees).
    //toAngleAxis() {
    //    Vec4.new()
    //}

    // Returns a formatted string for this quaternion.
    toString { "[%(x), %(y), %(z), %(w)]" }

    // Combines rotations this and other.
    foreign mulQuat(o)
    foreign mulVec3(o)
    *(o) {
        if (o is Vec3) {
            return mulVec3(o)
        }
        return mulQuat(o)
    }

    // Are two quaternions equal to each other?
    //==(other) { (other != null) && (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w) }
    //!=(other) { !(this == other) }

    // The identity rotation.
    static identity { Vec4.new(0, 0, 0, 1) }

    // The dot product between two rotations.
    //static dot(a, b) { a.x * b.x + a.y * b.y + a.z * b.z }

    // Converts this quaternion to one with the same orientation but with a magnitude of 1.
    //static normalize(v) { Vec3.new(v.x, v.y, v.z).normalized }

    // Returns the angle in degrees between two rotations a and b.
    //static angle(a, b) { 0 }

    // Creates a rotation which rotates angle degrees around axis.
    //static angleAxis(a, v) { Quat.new() }

    // Returns a rotation that rotates z degrees around the z axis, x degrees around the x axis, and y degrees around the y axis; applied in that order.
    foreign static euler(x, y, z)

    // Creates a rotation which rotates from a to b.
    //static fromToRotation(a, b) { Quat.new() }

    // Returns the Inverse of rotation.
    //static inverse(a, b) { Quat.new() }

    // Creates a rotation with the specified forward and upwards directions.
    //static lookRotation(f, u) { Quat.new() }

    // Rotates a rotation a towards b.
    //static rotateTowards(a, b, dd) { Quat.new() }

    // Interpolates between a and b by t and normalizes the result afterwards. The parameter t is not clamped.
    //static lerpUnclamped(a, b, t) { Quat.new() }

    // Interpolates between a and b by t and normalizes the result afterwards. The parameter t is clamped to the range [0, 1].
    //static lerp(a, b, t) { Quat.new() }

    // Spherically interpolates between a and b by t. The parameter t is not clamped.
    //static slerpUnclamped(a, b, t) { Quat.new() }

    // Spherically interpolates between quaternions a and b by ratio t. The parameter t is clamped to the range [0, 1].
    //static slerp(a, b, t) { Quat.new() }
}

// A standard 4x4 transformation matrix.
foreign class Mat4 {
    construct new(x, y, z, w) {}

    //foreign x
    //foreign x=(v)
//
    //foreign y
    //foreign y=(v)
//
    //foreign z
    //foreign z=(v)
//
    //foreign w
    //foreign w=(v)

    //foreign decomposeProjection
    //foreign determinant
    //foreign inverse
    //foreign isIdentity
    //foreign lossyScale
    //foreign rotation
    //[i, j] {}
    //foreign transpose

    foreign mulMat4(o)
    *(o) {
        return mulMat4(o)
    }

    static identity { Mat4.new(Vec4.new(1, 0, 0, 0), Vec4.new(0, 1, 0, 0), Vec4.new(0, 0, 1, 0), Vec4.new(0, 0, 0, 1)) }
    static zero { Mat4.new(Vec4.new(0, 0, 0, 0), Vec4.new(0, 0, 0, 0), Vec4.new(0, 0, 0, 0), Vec4.new(0, 0, 0, 0)) }

    //foreign getColumn(pos, q, s)
    //foreign getPosition(pos, q, s)
    //foreign getRow(pos, q, s)
    //foreign multiplyPoint(pos, q, s)
    //foreign multiplyPoint3x4(pos, q, s)
    //foreign multiplyVector(pos, q, s)
    //foreign setColumn(pos, q, s)
    //foreign setRow(pos, q, s)
    //foreign setTrs(pos, q, s)
    //foreign transformPlane(pos, q, s)
    //foreign validTrs(pos, rot, scl)

    //foreign static frustrum(pos, q, s)
    //foreign static inverse3DAffine(pos, q, s)
    foreign static lookAt(eye, center, up)
    foreign static ortho(left, right, bottom, top, zNear, zFar)
    foreign static perspective(fov, aspect, zNear, zFar)
    //foreign static scale(scl)
    //foreign static translate(pos)
    foreign static trs(pos, rot, scl)

    toString { "[%(x), %(y), %(z), %(w)]" }
}