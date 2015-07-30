#version 150
precision mediump float;

uniform int texCount;
uniform vec4 diffuseColor;
uniform vec4 ambientColor;
uniform sampler2D textureImage;
uniform samplerCube envMap;

in mediump vec2 texCoordFrag;
in mediump vec3 normalWorld;
in mediump vec3 lightDirWorld;

out vec4 outFrag;

void main()
{
    vec4 color;
    vec4 amb;
    float brightness;
        
    //calcuate lighting brightness
    float normalDotL = dot(normalWorld, lightDirWorld);
    brightness = max(normalDotL, 0.0);
    
    if( texCount == 2 ) {
        color = texture(textureImage, texCoordFrag);
        
        //calculate reflection, based on the untransformed normal.
        vec3 reflection = (2.0 * normalize(normalWorld) * normalDotL) - lightDirWorld;
        vec4 envColor = texture( envMap, reflection);
        
        //attenuate the src color plus environment color
        outFrag = 0.5 * vec4(color.rgb*brightness, color.a) + 0.5 * vec4(envColor.rgb*brightness, envColor.a);
    } else if( texCount == 1 ) {
        color = texture(textureImage, texCoordFrag);
        amb = color * 0.33;
        outFrag = vec4(color.rgb*brightness, color.a) + amb;
    } else if( texCount == 3 ) {
        //candide3 simple mask
        outFrag = texture(textureImage, texCoordFrag);
        //outFrag = vec4(0, 1, 0, 1); //green
    } else {
        color = diffuseColor;
        amb = ambientColor;
        outFrag = vec4(color.rgb*brightness, color.a) + amb;
    }
}