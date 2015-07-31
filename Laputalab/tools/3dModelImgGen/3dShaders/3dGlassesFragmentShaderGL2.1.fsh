#version 120

uniform int texCount;
uniform vec4 diffuseColor;
uniform vec4 ambientColor;
uniform vec4 specularColor;
uniform sampler2D textureImage;
uniform samplerCube envMap;

invariant varying vec2 texCoordFrag;
invariant varying vec3 normalWorld;
invariant varying vec3 lightDirWorld;

void main()
{
    vec4 color;
    vec4 amb;
    vec4 spec;
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
        amb = vec4(color.rgb * 0.33, color.a); //light's ambient coefficient=0.33
        spec = specularColor; //assume shininess=0, specularCoefficient = 1;
        gl_FragColor = vec4(color.rgb*brightness + amb.rgb, color.a);// + spec.rgb;
        //need gamma correction?
    } else if( texCount == 3 ) {
        //candide3 simple mask
        gl_FragColor = texture2D(textureImage, texCoordFrag);
        //outFrag = vec4(0, 1, 0, 1); //green
    } else {
        color = diffuseColor;
        amb = ambientColor;
        spec = specularColor; //assume shininess=0, specularCoefficient = 1;
        gl_FragColor = vec4(color.rgb*brightness + amb.rgb, color.a);// + spec.rgb;
    }
}