#version 120

attribute vec3 position;
attribute vec2 texCoord;
attribute vec3 normal;

uniform mat4 MVP;
uniform mat4 World;
uniform mat3 NormalMatrix;

invariant varying vec2 texCoordFrag;
invariant varying vec3 normalWorld;
invariant varying vec3 surfacePos;
invariant gl_Position;

void main(){
    gl_Position = MVP * vec4(position, 1.0);
    texCoordFrag   = texCoord;

    normalWorld = normalize(NormalMatrix * normal);

    surfacePos = vec3(World * vec4(position, 1));
}