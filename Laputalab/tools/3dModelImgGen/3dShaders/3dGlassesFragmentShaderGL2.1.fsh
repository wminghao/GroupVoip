#version 120

uniform int texCount;
uniform vec4 diffuseColor;
uniform vec4 ambientColor;
uniform vec4 specularColor;
uniform sampler2D textureImage;
uniform samplerCube envMap;

invariant varying vec2 texCoordFrag;
invariant varying vec3 normalWorld;
invariant varying vec3 surfacePos;

void main()
{
    vec3 diffuse;
    vec3 amb;
    vec3 spec;
    vec4 surfaceColor;
    
    //It's point light. color=white, different light for different angle.
    //calculate the vector from the light source to the vertex position.
    // object is initially located at z = -600 position
    vec3 cameraPosition = vec3(0, 0, 0.01);
    vec3 lightPosition = vec3(10, 10, 0);
    vec3 surfaceToLight = normalize( lightPosition - surfacePos );
    vec3 surfaceToCamera = normalize( cameraPosition - surfacePos );
    float distanceToLight = length( lightPosition - surfacePos );
    
    //calcuate lighting diffuseCoefficent(brightness)
    float normalDotL = dot(normalWorld, surfaceToLight);
    float diffuseCoefficent = max(normalDotL, 0.0);
    
    //calculate the specularCoefficient, shininess is alway 0, means max shininess = 1.
    float specularCoefficent = 0.0;
    if( diffuseCoefficent > 0 ) {
        specularCoefficent = pow(max(0.0, dot(surfaceToCamera, reflect(-surfaceToLight, normalWorld))), 1);
    }
    
    //light ambient coefficient
    float lightAmbientCoefficient = 0.33;
    
    //calculate attenuation
    float lightAttenuationFactor = 0.000001; //almost no effect,don't know why?
    float attenuation = 1.0/(1.0 + lightAttenuationFactor * pow(distanceToLight, 2));
    
    //gamma correction, does not seem to work, maybe unecessary?
    vec3 gamma = vec3(1.0/2.2);
    vec3 linearColor;
    
    if( texCount == 2 ) {
        surfaceColor = texture2D(textureImage, texCoordFrag);
        diffuse = diffuseCoefficent * surfaceColor.rgb;
        spec = specularCoefficent * specularColor.rgb;
        
        //calculate reflection, based on the untransformed normal.
        vec3 reflection = (2.0 * normalize(normalWorld) * normalDotL) - surfaceToLight;
        vec4 envColor = textureCube( envMap, reflection);
        
        //attenuate the src color plus environment color
        gl_FragColor = 0.5 * vec4(attenuation * (diffuse+spec), surfaceColor.a) + 0.5 * vec4(diffuseCoefficent * envColor.rgb, envColor.a);
    } else if( texCount == 1 ) {
        surfaceColor = texture2D(textureImage, texCoordFrag);
        diffuse = diffuseCoefficent * surfaceColor.rgb;
        amb =  lightAmbientCoefficient * surfaceColor.rgb; //light's ambient coefficient=0.33
        spec = specularCoefficent * specularColor.rgb;
        
        linearColor = attenuation * (diffuse+spec) + amb;
        gl_FragColor = vec4(linearColor, surfaceColor.a);
        //gl_FragColor = vec4(pow(linearColor, gamma), surfaceColor.a);
    } else if( texCount == 3 ) {
        //candide3 simple mask, do not display
        gl_FragColor = texture2D(textureImage, texCoordFrag);
        //outFrag = vec4(0, 1, 0, 1); //green
    } else {
        surfaceColor = diffuseColor;
        diffuse = diffuseCoefficent * surfaceColor.rgb;
        amb = lightAmbientCoefficient * ambientColor.rgb;
        spec = specularCoefficent * specularColor.rgb;
        
        linearColor = attenuation * (diffuse+spec) + amb;
        gl_FragColor = vec4(linearColor, surfaceColor.a);
        //gl_FragColor = vec4(pow(linearColor, gamma), surfaceColor.a);
    }
}