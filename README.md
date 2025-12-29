# **BitCache Labs Technology Stack (bx)**

The BitCache-Labs Tech Stack (bx) is a collection of modular, simple, and lightweight APIs designed to help developers build high-performance, scalable applications with ease.

## Modular Libraries
The bx stack is composed of modular libraries that can be built and used independently, provided their respective dependencies are available.

### bx_core
The foundational bx library, providing low-level memory abstractions and essential utility functionality.

### bx_math
A mathematics library supporting 2D, 3D, and plane-based Geometric Algebra (PGA), featuring cross-platform SIMD acceleration.  
**Dependencies:** bx_core

### bx_stl
A cross-platform, high-performance standard library inspired by the C++ STL.  
**Dependencies:** bx_core

### bx_type
A robust type system with reflection support.  
**Dependencies:** bx_core, bx_stl

### bx_app
The primary application programming interface for bx-based applications.  
**Dependencies:** bx_core, bx_math, bx_stl, bx_type

### bx_ecs
A fast and reliable Entity Component System (ECS) library.  
**Dependencies:** bx_core, bx_stl

### bx_physics
A physics engine built on a novel hybrid approach combining classical linear algebra with plane-based geometric algebra.  
**Dependencies:** bx_core, bx_math, bx_stl

### bx_engine
A modern, high-performance engine for real-time applications, built on top of the bx ecosystem.  
**Dependencies:** bx_core, bx_math, bx_stl, bx_type, bx_app, bx_ecs, bx_physics