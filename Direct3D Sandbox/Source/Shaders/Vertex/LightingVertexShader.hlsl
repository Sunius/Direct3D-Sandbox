cbuffer MatrixBuffer
{
	matrix worldViewProjectionMatrix;
	matrix inversedTransposedWorldMatrix;
};

struct VertexInput
{
    float4 position : POSITION;
	float2 tex : TEXTURECOORDINATES;
	float3 normal : NORMAL;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 tex : TEXTURECOORDINATES;
	float3 normal : NORMAL;
};

PixelInput main(VertexInput input)
{
	PixelInput output;
	
	output.position = mul(input.position, worldViewProjectionMatrix);
	
	output.tex = input.tex;
    output.normal = -mul(input.normal, (float3x3)inversedTransposedWorldMatrix);

	return output;
}