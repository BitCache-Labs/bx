#include <engine/math.hpp>
#include <engine/script.hpp>

#ifdef MATH_API
static const StringView g_mathSrc = R"(
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
)";

BX_SCRIPT_API_REGISTRATION(Math)
{
	Script::Get().BeginModule("math");
	{
		//Script::Get().BeginClass("Math");
		//{
		//	//Script::Get().BindFunction(true, "test()", [](ScriptHandle vm) { Audio::Get().Test(); });
		//}
		//Script::Get().EndClass();
	}
	Script::Get().EndModule();

	ScriptModuleSource src{};
	src.moduleName = "math";
	src.moduleSource = g_mathSrc;
	return src;
}
#endif

#define GLM_LANG_STL11_FORCED
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_QUAT_DATA_XYZW

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

// FIXME: There is a bug with glm when GLM_FORCE_QUAT_DATA_XYZW is defined
#define GLM_PATCH_QUAT_DATA_XYZW

#ifdef GLM_PATCH_QUAT_DATA_XYZW
static Quat QuatFromGLM(glm::quat q)
{
	return Quat(q.x, q.y, q.z, q.w);
}

static glm::quat QuatToGLM(const Quat& q)
{
	return glm::quat(q.w, q.x, q.y, q.z);
}
#else
static Quat QuatFromGLM(glm::quat q)
{
	return Quat::FromValuePtr(glm::value_ptr(q));
}

static glm::quat QuatToGLM(const Quat& q)
{
	return glm::make_quat(q.data);
}
#endif

f32 Vec2::SqrMagnitude()
{
	glm::vec2 v = glm::make_vec2(data);
	return glm::dot(v, v);
}

f32 Vec2::Magnitude()
{
	glm::vec2 v = glm::make_vec2(data);
	return glm::length(v);
}

Vec2 Vec2::Normalized()
{
	glm::vec2 v = glm::normalize(glm::make_vec2(data));
	return Vec2(v.x, v.y);
}

f32 Vec2::At(i32 i)
{
	return data[i];
}

void Vec2::Set(f32 x, f32 y)
{
	data[0] = x; data[1] = y;
}

Vec2 Vec2::Plus(const Vec2& rhs) const
{
	glm::vec2 v = glm::make_vec2(data) + glm::make_vec2(rhs.data);
	return Vec2(v.x, v.y);
}

Vec2 Vec2::Negate() const
{
	return Vec2(-x, -y);
}

Vec2 Vec2::Minus(const Vec2& rhs) const
{
	glm::vec2 v = glm::make_vec2(data) - glm::make_vec2(rhs.data);
	return Vec2(v.x, v.y);
}

Vec2 Vec2::Mul(f32 rhs) const
{
	glm::vec2 v = glm::make_vec2(data) * rhs;
	return Vec2(v.x, v.y);
}

Vec2 Vec2::Div(f32 rhs) const
{
	glm::vec2 v = glm::make_vec2(data) / rhs;
	return Vec2(v.x, v.y);
}

f32 Vec2::Dot(const Vec2& a, const Vec2& b)
{
	return glm::dot(glm::make_vec2(a.data), glm::make_vec2(b.data));
}

void Vec2::Normalize(Vec2& v)
{
	v = v.Normalized();
}

//Vec2 Vec2::Cross(const Vec2& a, const Vec2& b)
//{
//	glm::vec2 v = glm::cross(glm::make_vec2(a.data), glm::make_vec2(b.data));
//	return Vec2(v.x, v.y);
//}

Vec2 Vec2::Lerp(const Vec2& a, const Vec2& b, f32 t)
{
	glm::vec2 va = glm::make_vec2(a.data);
	glm::vec2 vb = glm::make_vec2(b.data);
	glm::vec2 vl = glm::mix(va, vb, t);
	return Vec2::FromValuePtr(glm::value_ptr(vl));
}

Vec2 Vec2::FromValuePtr(f32* vptr)
{
	Vec2 v;
	memcpy(v.data, vptr, sizeof(Vec2));
	return v;
}

f32 Vec3::SqrMagnitude()
{
	glm::vec3 v = glm::make_vec3(data);
	return glm::dot(v, v);
}

f32 Vec3::Magnitude()
{
	glm::vec3 v = glm::make_vec3(data);
	return glm::length(v);
}

Vec3 Vec3::Normalized()
{
	glm::vec3 v = glm::normalize(glm::make_vec3(data));
	return Vec3(v.x, v.y, v.z);
}

f32 Vec3::At(i32 i)
{
	return data[i];
}

void Vec3::Set(f32 x, f32 y, f32 z)
{
	data[0] = x; data[1] = y; data[2] = z;
}

Vec3 Vec3::Plus(const Vec3& rhs) const
{
	glm::vec3 v = glm::make_vec3(data) + glm::make_vec3(rhs.data);
	return Vec3(v.x, v.y, v.z);
}

Vec3 Vec3::Negate() const
{
	return Vec3(-x, -y, -z);
}

Vec3 Vec3::Minus(const Vec3& rhs) const
{
	glm::vec3 v = glm::make_vec3(data) - glm::make_vec3(rhs.data);
	return Vec3(v.x, v.y, v.z);
}

Vec3 Vec3::Mul(f32 rhs) const
{
	glm::vec3 v = glm::make_vec3(data) * rhs;
	return Vec3(v.x, v.y, v.z);
}

Vec3 Vec3::Div(f32 rhs) const
{
	glm::vec3 v = glm::make_vec3(data) / rhs;
	return Vec3(v.x, v.y, v.z);
}

f32 Vec3::Dot(const Vec3& a, const Vec3& b)
{
	return glm::dot(glm::make_vec3(a.data), glm::make_vec3(b.data));
}

void Vec3::Normalize(Vec3& v)
{
	v = v.Normalized();
}

Vec3 Vec3::Cross(const Vec3& a, const Vec3& b)
{
	glm::vec3 v = glm::cross(glm::make_vec3(a.data), glm::make_vec3(b.data));
	return Vec3(v.x, v.y, v.z);
}

Vec3 Vec3::Lerp(const Vec3& a, const Vec3& b, f32 t)
{
	glm::vec3 va = glm::make_vec3(a.data);
	glm::vec3 vb = glm::make_vec3(b.data);
	glm::vec3 vl = glm::mix(va, vb, t);
	return Vec3::FromValuePtr(glm::value_ptr(vl));
}

Vec3 Vec3::FromValuePtr(f32* vptr)
{
	Vec3 v;
	memcpy(v.data, vptr, sizeof(Vec3));
	return v;
}

f32 Vec4::SqrMagnitude()
{
	glm::vec3 v = glm::make_vec3(data);
	return glm::dot(v, v);
}

f32 Vec4::Magnitude()
{
	glm::vec3 v = glm::make_vec3(data);
	return glm::length(v);
}

Vec4 Vec4::Normalized()
{
	glm::vec4 v = glm::normalize(glm::make_vec4(data));
	return Vec4(v.x, v.y, v.z, v.w);
}

f32 Vec4::At(i32 i)
{
	return data[i];
}

void Vec4::Set(f32 x, f32 y, f32 z, f32 w)
{
	data[0] = x; data[1] = y; data[2] = z; data[3] = w;
}

Vec4 Vec4::Plus(const Vec4& rhs) const
{
	glm::vec4 v = glm::make_vec4(data) + glm::make_vec4(rhs.data);
	return Vec4(v.x, v.y, v.z, v.w);
}

Vec4 Vec4::Negate() const
{
	return Vec4(-x, -y, -z, -w);
}

Vec4 Vec4::Minus(const Vec4& rhs) const
{
	glm::vec4 v = glm::make_vec4(data) - glm::make_vec4(rhs.data);
	return Vec4(v.x, v.y, v.z, v.w);
}

Vec4 Vec4::Mul(f32 rhs) const
{
	glm::vec4 v = glm::make_vec4(data) * rhs;
	return Vec4(v.x, v.y, v.z, v.w);
}

Vec4 Vec4::Div(f32 rhs) const
{
	glm::vec4 v = glm::make_vec4(data) / rhs;
	return Vec4(v.x, v.y, v.z, v.w);
}

Vec4 Vec4::FromValuePtr(f32* vptr)
{
	Vec4 v;
	memcpy(v.data, vptr, sizeof(Vec4));
	return v;
}

Vec4i Vec4i::FromValuePtr(i32* vptr)
{
	Vec4i v;
	memcpy(v.data, vptr, sizeof(Vec4i));
	return v;
}

Quat Quat::Normalized()
{
	glm::quat q = glm::normalize(QuatToGLM(*this));
	return QuatFromGLM(q);
}

Quat Quat::MulQuat(Quat rhs) const
{
	glm::quat l = QuatToGLM(*this);
	glm::quat r = QuatToGLM(rhs);
	glm::quat q = l * r;
	return QuatFromGLM(q);
}

Vec3 Quat::MulVec3(Vec3 rhs) const
{
	glm::quat l = QuatToGLM(*this);
	glm::vec3 r = glm::make_vec3(rhs.data);
	glm::vec3 v = l * r;
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Quat::EulerAngles() const
{
	glm::quat q = QuatToGLM(*this);
	glm::vec3 e = glm::degrees(glm::eulerAngles(q));
	return Vec3::FromValuePtr(glm::value_ptr(e));
}

Quat Quat::Inverse() const
{
	glm::quat q = QuatToGLM(*this);
	glm::quat iq = glm::inverse(q);
	return QuatFromGLM(iq);
}

Quat Quat::Euler(f32 x, f32 y, f32 z)
{
	glm::quat q = glm::quat(glm::radians(glm::vec3(x, y, z)));
	return QuatFromGLM(q);
}

Quat Quat::AngleAxis(f32 a, const Vec3& axis)
{
	glm::quat q = glm::angleAxis(glm::radians(a), glm::vec3(axis.x, axis.y, axis.z));
	return QuatFromGLM(q);
}

void Quat::Normalize(Quat& q)
{
	q = q.Normalized();
}

Quat Quat::Slerp(const Quat& a, const Quat& b, f32 t)
{
	glm::quat qa = QuatToGLM(a);
	glm::quat qb = QuatToGLM(b);
	glm::quat qs = glm::slerp(qa, qb, t);
	return QuatFromGLM(qs);
}

Quat Quat::FromValuePtr(f32* vptr)
{
	Quat q;
	memcpy(q.data, vptr, sizeof(Quat));
	return q;
}

Mat4 Mat4::Mul(const Mat4& rhs) const
{
	glm::mat4 l = glm::make_mat4(data);
	glm::mat4 r = glm::make_mat4(rhs.data);
	glm::mat4 m = l * r;
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Inverse() const
{
	glm::mat4 m = glm::make_mat4(data);
	glm::mat4 im = glm::inverse(m);
	return Mat4::FromValuePtr(glm::value_ptr(im));
}

Mat4 Mat4::Identity()
{
	static Mat4 s_identity = Mat4(Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0), Vec4(0, 0, 0, 1));
	return s_identity;
}

Mat4 Mat4::Zero()
{
	return Mat4(Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0));
}

Mat4 Mat4::LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
	glm::vec3 e = glm::make_vec3(eye.data);
	glm::vec3 c = glm::make_vec3(center.data);
	glm::vec3 u = glm::make_vec3(up.data);
	glm::mat4 m = glm::lookAt(e, c, u);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar)
{
	glm::mat4 m = glm::ortho(left, right, bottom, top, zNear, zFar);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Perspective(f32 fov, f32 aspect, f32 zNear, f32 zFar)
{
	glm::mat4 m = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::TRS(const Vec3& pos, const Quat& rot, const Vec3& scl)
{
	/* This creates a mat4 directly from pos, euler, scale
	r = rad(r)
		var cr = r.map { |n| n.cos }.toList
		var sr = r.map { |n| n.sin }.toList

		return [s[0] * ( cr[1] * cr[2]),    s[1] * (sr[0] * sr[1] * cr[2] - cr[0] * sr[2]),     s[2] * (cr[0] * sr[1] * cr[2] + sr[0] * sr[2]),     0,
				s[0] * ( cr[1] * sr[2]),    s[1] * (sr[0] * sr[1] * sr[2] + cr[0] * cr[2]),     s[2] * (cr[0] * sr[1] * sr[2] - sr[0] * cr[2]),     0,
				s[0] * (-sr[1]),            s[1] * (sr[0] * cr[1]),                             s[2] * (cr[0] * cr[1]),                             0,
				p[0],                       p[1],                                               p[2],                                               1]
	*/

	glm::vec3 p = glm::make_vec3(pos.data);
	glm::quat r = QuatToGLM(rot);
	glm::vec3 s = glm::make_vec3(scl.data);
	glm::mat4 m = glm::translate(glm::mat4(1.0f), p) * glm::mat4(r) * glm::scale(glm::mat4(1.0f), s);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Translate(const Vec3& translation, const Mat4& mat)
{
	glm::vec3 t = glm::make_vec3(translation.data);
	glm::mat4 m = glm::make_mat4(mat.data);
	glm::mat4 tm = glm::translate(m, t);
	return Mat4::FromValuePtr(glm::value_ptr(tm));
}

Mat4 Mat4::Rotate(const Quat& rotation, const Mat4& mat)
{
	glm::quat r = QuatToGLM(rotation);
	glm::mat4 m = glm::make_mat4(mat.data);
	glm::mat4 rm = m * glm::mat4(r);
	return Mat4::FromValuePtr(glm::value_ptr(rm));
}

Mat4 Mat4::Scale(const Vec3& scaling, const Mat4& mat)
{
	glm::vec3 s = glm::make_vec3(scaling.data);
	glm::mat4 m = glm::make_mat4(mat.data);
	glm::mat4 sm = glm::scale(m, s);
	return Mat4::FromValuePtr(glm::value_ptr(sm));
}

void Mat4::Decompose(const Mat4& m, Vec3& pos, Quat& rot, Vec3& scl)
{
	glm::mat4 transformation = glm::make_mat4(m.data);
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transformation, scale, rotation, translation, skew, perspective);
	pos = Vec3::FromValuePtr(glm::value_ptr(translation));
	rot = QuatFromGLM(rotation);
	scl = Vec3::FromValuePtr(glm::value_ptr(scale));
}

Mat4 Mat4::FromValuePtr(f32* vptr)
{
	Mat4 m;
	memcpy(m.data, vptr, sizeof(Mat4));
	return m;
}