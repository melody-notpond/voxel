struct UniformBuffer {
  float4x4 model;
  float4x4 view;
  float4x4 proj;
}

struct VsInput {
  float3 pos;
  float3 color;
  float2 texCoord;
}

struct VsOutput {
  float4 sv_position: SV_Position;
  float3 color;
  float2 texCoord;
};

ConstantBuffer<UniformBuffer> ubo;

[shader("vertex")]
VsOutput vert_main(VsInput input) {
  VsOutput output;
  float4x4 mat = mul(ubo.proj, mul(ubo.view, ubo.model));
  output.sv_position = mul(mat, float4(input.pos, 1.0));
  output.color = input.color;
  output.texCoord = input.texCoord;
  return output;
}

Sampler2D texture;

[shader("fragment")]
float4 frag_main(VsOutput vin) : SV_Target {
  // return float4(vin.color, 1.0);
  return float4(vin.color * texture.Sample(vin.texCoord).rgb, 1.);
}
