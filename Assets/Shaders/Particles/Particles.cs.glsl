#version 430

layout (local_size_x = 1000) in;

// black hole #1
uniform float Gravity1 = 1000.0;
uniform vec3 BlackHole1Pos = vec3 (5.0, 0.0, 0.0);

// block hole #2
uniform float Gravity2 = 1000.0;
uniform vec3 BlackHole2Pos = vec3 (-5.0, 0.0, 0.0);

// particles
uniform float ParticleInvMass = 1.0 / 0.1;
uniform float DeltaTime = 0.0005;
uniform float MaxDist = 45.0;

// out
layout (std430, binding = 0) buffer Pos {
    vec4 Position[];
};
layout (std430, binding = 1) buffer Vel {
    vec4 Velocity[];
};

void main (void) {

    const uint idx = gl_GlobalInvocationID.x;
    const vec3 p = Position[idx].xyz;

    // Force from block hole #1.
    const vec3 delta1 = BlackHole1Pos - p;
    const float dist1 = length (delta1);
    const vec3 force1 = (Gravity1 / dist1) * normalize (delta1);

    // Force from block hole #2.
    const vec3 delta2 = BlackHole2Pos - p;
    const float dist2 = length (delta2);
    const vec3 force2 = (Gravity2 / dist2) * normalize (delta2);

    // Reset particles that get too far from the hole.
    if (dist1 > MaxDist) {
        Position[idx] = vec4 (0.0, 0.0, 0.0, 1.0);
    }
    // Apply euler intergrator.
    else {
        const vec3 force = force1 + force2;
        const vec3 acceleration = force * ParticleInvMass;
        Position[idx] = vec4 (p + Velocity[idx].xyz * DeltaTime + 0.5 * acceleration * DeltaTime * DeltaTime, 1.0);
        Velocity[idx] = vec4 (Velocity[idx].xyz + acceleration * DeltaTime, 0.0);
    }
}
