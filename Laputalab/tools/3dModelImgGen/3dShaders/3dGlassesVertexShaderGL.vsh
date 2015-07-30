#version 150

in vec3 position;
in mediump vec2 texCoord;
in vec3 normal;

uniform mat4 MVP;
uniform mat4 World;
uniform mat3 NormalMatrix;

out mediump vec2 texCoordFrag;
out mediump vec3 normalWorld;
out mediump vec3 lightDirWorld;

void main(){
    gl_Position = MVP * vec4(position, 1.0);
    texCoordFrag   = texCoord;

    normalWorld = normalize(NormalMatrix * normal);

    //It's point light. different light for different angle.
    //calculate the vector from the light source to the vertex position.
    //light position is in camera position, 0, 0, 0
    lightDirWorld = normalize(vec3(0, 0, 0) - vec3(World * vec4(position, 1))); //no attenuation
}