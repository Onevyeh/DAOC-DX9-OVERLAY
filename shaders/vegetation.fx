// vegetation.fx

float4x4 WorldViewProjection;
float time;

struct VS_INPUT {
    float4 pos : POSITION;
    float2 tex : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 pos : POSITION;
    float2 tex : TEXCOORD0;
};

VS_OUTPUT VS(VS_INPUT input) {
    VS_OUTPUT output;
    float offset = sin(input.pos.x * 5 + time) * 0.05;
    input.pos.x += offset;

    output.pos = mul(input.pos, WorldViewProjection);
    output.tex = input.tex;
    return output;
}

sampler TextureSampler = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
};

float4 PS(VS_OUTPUT input) : COLOR {
    return tex2D(TextureSampler, input.tex);
}

technique wind {
    pass P0 {
        VertexShader = compile vs_2_0 VS();
        PixelShader  = compile ps_2_0 PS();
    }
}
