#version 130

uniform vec4 color;

uniform vec3 lightDir;
uniform vec3 lightInten;
uniform vec3 lightAmb;

uniform int fogOn;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogStart;

uniform int   oceanColoringOn;
uniform vec3  oceanTopBrightness;
uniform vec3  oceanBottomBrightness;
uniform vec3  oceanTopColor;
uniform vec3  oceanBottomColor;
uniform float oceanDensity;
uniform float surfaceDepth;
uniform float floorDepth;

uniform int mode;

uniform int shouldTexture;
uniform sampler2D texSampler;

varying vec3 fragmentNormal;
varying vec2 fragmentTexCoord;
varying float distToCam;
varying float depth;

varying vec4 worldpos;

void main(){

    if(mode == 1){
        vec3 norm = normalize(fragmentNormal);
        vec4 colorLight = vec4(0.0,0.0,0.0,0.0);
        colorLight = texture(texSampler, fragmentTexCoord);

        vec3 lightDirn = vec3(-0.707,-0.707,0);

        vec4 sunlightMod = vec4((lightAmb + 0.5) + max(0, dot(-lightDirn, norm)) * lightInten,1.0);  // phong

        // Fog
        if(fogOn != 0 && -distToCam > fogStart){
            float ffog = exp2(-1.442695 * fogDensity * fogDensity * distToCam * distToCam);
            vec3 fog = (1.0 - ffog) * fogColor;
            colorLight = ffog * colorLight + vec4(fog, 1.0);
        }


        // Ocean Colors
        if(oceanColoringOn!=0) {

            // base weights
            vec3 bright = vec3(1.0,1.0,1.0); //container for final brightness based on depth
            vec3 col = vec3(1.0,1.0,1.0); //container for final color based on depth
            float ratio;
            // linearly interpolate between top and bottom values
            if(depth > surfaceDepth) {
                bright = oceanTopBrightness * bright;
                col    = oceanTopColor * col;
            } else if(depth < floorDepth) {
                bright = oceanBottomBrightness *  bright;
                col    = oceanBottomColor * col;
            } else {
                ratio  = (depth-floorDepth) / (surfaceDepth - floorDepth);
                bright = ratio * (oceanTopBrightness - oceanBottomBrightness) + oceanBottomBrightness;
                col    = ratio * (oceanTopColor - oceanBottomColor) + oceanBottomColor;
            }

            // do fog calculation
            float ffogOcean = exp2(-1.442695 * oceanDensity * oceanDensity * distToCam * distToCam);
            vec3 fogOcean = (1.0 - ffogOcean) * col;
            vec4 colorOcean = ffogOcean * vec4(1.0,1.0,1.0,1.0) + vec4(fogOcean, 1.0);

            //blend ocean color with mesh color from texture / base color
            float blendWeightFog = 1.0;
            colorLight = blendWeightFog*colorOcean*colorLight + (1.0-blendWeightFog)*colorLight;

            colorLight = colorLight * vec4(bright,1.0);
        }


        colorLight = colorLight * sunlightMod; // apply sun and diffuse

        // Out
        gl_FragColor = colorLight;
    }
    else if(mode == 2){
        vec3 fogcolor  = vec3(0.8,0.6,0.6);
        vec3 worldpos3 = vec3(worldpos.x/worldpos.w,worldpos.y/worldpos.w,worldpos.z/worldpos.w);
        vec4 texcol = texture(texSampler,fragmentTexCoord);
        if(worldpos3.y<0.2){
            gl_FragColor = mix(texcol,vec4(fogcolor,1), min((0.2-worldpos3.y)*5.0,1.0));
        }
        else gl_FragColor = texcol;
    }
}