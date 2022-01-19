#version 430 core

layout(location = 0) out vec4 o_Color;

in vec2 o_TexCoord;

uniform vec3 u_CameraPosition;
uniform vec3 u_DirLightDirection;
uniform float u_DirLightIntensity;
uniform vec3 u_DirLightCol;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_nearPlane;
uniform float u_farPlane;
uniform float u_AmbientIntensity;
uniform float u_timeAccum;

layout(binding = 0) uniform sampler2DArray shadowMap;
layout(binding = 1) uniform sampler2D depthMap;

uniform int u_cascadeCount;
uniform float u_cascadePlaneDistances[20];

struct PointLight{
    vec4 position;
    vec4 color;
    float intensity;
    float range;
};

layout (std430, binding = 0) buffer pointLightsSSBO
{
    PointLight pointLights[];
};

uniform uint u_PointLightCount;

layout (std430, binding = 1) buffer LightSpaceMatrices
{
    mat4 lightSpaceMatrices[];
};


//Settings
uniform int u_Samples = 50;
uniform float u_Gfactor = -0.1;
uniform float u_FogStrength = 0.8;
uniform float u_FogY = 5;

float dither_pattern[16] = float[16] (
	0.0, 0.5, 0.125, 0.625,
	0.75, 0.22, 0.875, 0.375,
	0.1875, 0.6875, 0.0625, 0.5625,
	0.9375, 0.4375, 0.8125, 0.3125
);

vec3 reconstructPosition(vec2 uv, float z, mat4 invVP)
{
    vec4 position_s = vec4(uv * 2.0 - vec2(1.0), 2.0 * z - 1.0, 1.0);
    vec4 position_v = invVP * position_s;
    return position_v.xyz / position_v.w;
}
float ComputeScattering(float lightDotView)
{
    float result = 1.0f - u_Gfactor * u_Gfactor;
    result /= (4.0f * 3.14159265359 * pow(1.0f + u_Gfactor * u_Gfactor - (2.0f * u_Gfactor) * lightDotView, 1.5f));
    return result;
}
float dirLightShadow(vec3 fPos);
float sample_fog(vec3 pos, float time);
void main()
{
    float depth = texture(depthMap, o_TexCoord, 0).r;
    vec3 worldPos = reconstructPosition(o_TexCoord, depth, inverse(u_Projection * u_View));

    float dither_value = dither_pattern[ (int(gl_FragCoord.x) % 4)* 4 + (int(gl_FragCoord.y) % 4) ];


    vec3 startPosition = u_CameraPosition;

    vec3 rayVector = worldPos - startPosition;

    float rayLength = length(rayVector);
    vec3 rayDirection = rayVector / rayLength;

    float stepLength = rayLength / u_Samples;

    vec3 oneStep = rayDirection * stepLength;
    startPosition+=oneStep;
    
    // dither only when the camera is not pointing close to the sun
    if(dot(normalize(rayDirection), normalize(u_CameraPosition*u_DirLightDirection))<0.7)
        startPosition+=dither_value;

    vec3 currentPosition = startPosition;


    vec3 L = vec3(0.0, 0.0, 0.0);

    float extraFog = 0;
    float dirScatter = ComputeScattering(dot(normalize(rayDirection), normalize(-u_DirLightDirection)));
    float pointScatters[1000]; //FIXME: should be u_PointLightCount
    for(int i=0; i<1; i++)
        pointScatters[i] = ComputeScattering(dot(normalize(rayDirection), normalize(startPosition-pointLights[i].position.xyz)));


    if(worldPos.y < u_FogY)
        extraFog = pow(mix(u_FogStrength, 0, clamp(worldPos.y/u_FogY, 0, 1)), 2);

    for (int i = 0; i < u_Samples; i++) 
    { 
        float shadow = dirLightShadow(currentPosition);

        float fog = extraFog*pow(sample_fog(currentPosition+.8-float(i)*0.1, u_timeAccum), 1); //TODO: replace with nicer noise texture

        if(shadow != 1 )
            L += (dirScatter*u_DirLightCol+(fog*u_DirLightCol))*u_DirLightIntensity;
        L += fog*u_AmbientIntensity;

        for (int i = 0; i < u_PointLightCount; i++) 
        { 
            PointLight light = pointLights[i];
            float dist = length(currentPosition-light.position.xyz);
            L += ((pointScatters[i]*light.color.rgb+(fog*light.color.rgb))*light.intensity)*clamp(1/(dist*dist)-0.01, 0.0001, 1.0);
        }

        currentPosition += oneStep;
    }
    o_Color = vec4(L/u_Samples, 1);
    // o_Color = vec4(vec3(sample_fog(worldPos.xyz, u_timeAccum)), 1);
    // o_Color = vec4(rayCoord, 1);
}

float dirLightShadow(vec3 fPos)
{
    vec4 pos = u_View * vec4(fPos, 1.0);
    float depth = abs(pos.z);

    int layer = -1;
    // layer = 1;
    for (int i = 0; i < u_cascadeCount; ++i)
        if (depth < u_cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }

    if (layer == -1)
        layer = u_cascadeCount;
    
    vec4 posLightSpace = lightSpaceMatrices[layer] * vec4(fPos, 1.0);
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth  > 1.0)
        return 0.0;

    // PCF
    float shadow = 0.0;

    float sDepth = texture(shadowMap, vec3(projCoords.xy, layer)).r; 
    shadow = currentDepth > sDepth ? 1.0 : 0.0;        

    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

float tri( float x ){ 
  return abs( fract(x) - .5 );
}

vec3 tri3( vec3 p ){
 
  return vec3( 
      tri( p.z + tri( p.y * 1. ) ), 
      tri( p.z + tri( p.x * 1. ) ), 
      tri( p.y + tri( p.x * 1. ) )
  );

}

// Taken from https://www.shadertoy.com/view/4ts3z2
// By NIMITZ  (twitter: @stormoid)
float triNoise3d(in vec3 p, in float spd, in float time)
{
    float z=1.4;
	float rz = 0.;
    vec3 bp = p;
	for (float i=0.; i<=3.; i++ )
	{
        vec3 dg = tri3(bp*2.);
        p += (dg+time*spd);

        bp *= 1.8;
		z *= 1.5;
		p *= 1.2;
        //p.xz*= m2;
        
        rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
        bp += 0.14;
	}
	return rz;
}

float sample_fog(vec3 pos, float time) 
{
	return triNoise3d(pos/50, 0.1, time);
}