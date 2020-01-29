cbuffer VS_BLOOM_PARAMETERS : register(b0)
{
    float BloomThreshold;
    float BlurAmount;
    float BloomIntensity;
    float BaseIntensity;
    float BloomSaturation;
    float BaseSaturation;
}