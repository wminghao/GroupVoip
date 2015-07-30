#version 120

uniform int texCount;
uniform vec4 diffuseColor;
uniform vec4 ambientColor;
uniform sampler2D textureImage;
uniform samplerCube envMap;

invariant varying vec2 texCoordFrag;
invariant varying vec3 normalWorld;
invariant varying vec3 lightDirWorld;

void main()
{
    vec4 color;
    vec4 amb;
    float brightness;
        
    //calcuate lighting brightness
    float normalDotL = dot(normalWorld, lightDirWorld);
    brightness = max(normalDotL, 0.0);
    
    if( texCount == 2 ) {
        color = texture2D(textureImage, texCoordFrag);
        
        //calculate reflection, based on the untransformed normal.
        vec3 reflection = (2.0 * normalize(normalWorld) * normalDotL) - lightDirWorld;
        vec4 envColor = textureCube( envMap, reflection);
        
        //attenuate the src color plus environment color
        gl_FragColor = 0.5 * vec4(color.rgb*brightness, color.a) + 0.5 * vec4(envColor.rgb*brightness, envColor.a);
    } else if( texCount == 1 ) {
        color = texture2D(textureImage, texCoordFrag);
        amb = color * 0.33;
        gl_FragColor = vec4(color.rgb*brightness, color.a) + amb;
    } else if( texCount == 3 ) {
        //candide3 simple mask
        gl_FragColor = texture2D(textureImage, texCoordFrag);
        //outFrag = vec4(0, 1, 0, 1); //green
    } else {
        color = diffuseColor;
        amb = ambientColor;
        gl_FragColor = vec4(color.rgb*brightness, color.a) + amb;
    }
}