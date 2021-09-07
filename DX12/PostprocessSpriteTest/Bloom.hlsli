cbuffer VS_BLOOM_PARAMETERS : register(b1)
{
    float BloomThreshold;
    float BlurAmount;
    float BloomIntensity;
    float BaseIntensity;
    float BloomSaturation;
    float BaseSaturation;
}

#define CustomSpriteRS \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS )," \
"DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_PIXEL ),"\
"CBV(b0), "\
"DescriptorTable ( SRV(t1), visibility = SHADER_VISIBILITY_PIXEL ),"\
"CBV(b1), "\
"StaticSampler(s0,"\
"           filter = FILTER_MIN_MAG_MIP_LINEAR,"\
"           addressU = TEXTURE_ADDRESS_CLAMP,"\
"           addressV = TEXTURE_ADDRESS_CLAMP,"\
"           addressW = TEXTURE_ADDRESS_CLAMP,"\
"           visibility = SHADER_VISIBILITY_PIXEL )"