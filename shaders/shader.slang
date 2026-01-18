static float3 vertices[8] = {
  float3(1, 1, 0), float3(1, 0, 0),
  float3(1, 1, 1), float3(1, 0, 1),
  float3(0, 1, 0), float3(0, 0, 0),
  float3(0, 1, 1), float3(0, 0, 1),
};

static uint indices[] = {
  4, 2, 0, 2, 7, 3, 6, 5, 7, 1, 7, 5,
  0, 3, 1, 4, 1, 5, 4, 6, 2, 2, 6, 7,
  6, 4, 5, 1, 3, 7, 0, 2, 3, 4, 0, 1
};

struct Camera {
  float4x4 proj_view;
  float4x4 view_inv;
  float4x4 proj_view_inv;
  float2 viewport;
  float tan_fov;
  float z_near;
  float z_far;
  uint max_marches;
}

struct Chunk {
  float4x4 model;
  float4x4 model_inv;
  uint voxel_count;
}

ConstantBuffer<Camera> cam;
ConstantBuffer<Chunk> chunk;
Texture3D<uint> voxels;

[shader("vertex")]
float4 vert_main(uint id : SV_VertexID) : SV_Position {
  float4 p = float4(vertices[indices[id]], 1.0);
  return mul(cam.proj_view, mul(chunk.model, p));
}

int3 pos_to_voxel_coords(float3 pos) {
  return int3(pos * chunk.voxel_count);
}

void next_voxel_plane(inout int3 voxel_pos, int3 step, inout float3 t_max,
  float3 t_delta, out float3 normal) {
  if (t_max.x < t_max.y) {
    if (t_max.x < t_max.z) {
      t_max.x += t_delta.x;
      voxel_pos.x += step.x;
      normal = float3(-step.x, 0, 0);
    } else {
      t_max.z += t_delta.z;
      voxel_pos.z += step.z;
      normal = float3(0, 0, -step.z);
    }
  } else {
    if (t_max.y < t_max.z) {
      t_max.y += t_delta.y;
      voxel_pos.y += step.y;
      normal = float3(0, -step.y, 0);
    } else {
      t_max.z += t_delta.z;
      voxel_pos.z += step.z;
      normal = float3(0, 0, -step.z);
    }
  }
}

float bound(float s, float ds) {
  if (ds < 0) {
    s = -s;
    ds = -ds;
  }

  // stolen from https://gamedev.stackexchange.com/a/49423
  s = (s % (1. / chunk.voxel_count) + 1. / chunk.voxel_count) %
    (1. / chunk.voxel_count);
  return (1. / chunk.voxel_count - s) / ds;
}

float3 bound(float3 s, float3 ds) {
  return float3(bound(s.x, ds.x), bound(s.y, ds.y), bound(s.z, ds.z));
}

// returns the voxel coordinate in .xyz and the voxel data in .w
// uses dda traversal
uint4 raymarch(float3 pos, float3 dir, uint limit, float start, float end,
  out float3 normal) {
  float3 origin = pos + start * dir;
  int3 voxel_pos = pos_to_voxel_coords(origin);
  int3 step = sign(dir);
  float3 t_max = bound(origin, dir);
  float3 t_delta = step / dir / chunk.voxel_count;

  for (int i = 0; i < limit * 10; i++) {
    uint voxel = voxels.Load(int4(voxel_pos, 0));
    if (voxel != 0)
      return uint4(voxel_pos, voxel);

    next_voxel_plane(voxel_pos, step, t_max, t_delta, normal);
  }
  return uint4(voxel_pos, 0);
}

[shader("fragment")]
float4 frag_main(in float4 pos : SV_Position)
  : SV_Target {
  float2 uv = (2 * pos.xy - cam.viewport) / cam.viewport;

  float3 ray_origin = mul(mul(chunk.model_inv, cam.view_inv),
    float4(0, 0, 0, 1)).xyz;
  float4 ray_dir4 = mul(mul(chunk.model_inv, cam.proj_view_inv),
    float4(uv, 0, 1));
  float3 ray_dir = normalize(ray_dir4.xyz / ray_dir4.w - ray_origin);
  float3 normal;

  uint4 voxel_data = raymarch(ray_origin, ray_dir, cam.max_marches, cam.z_near,
    cam.z_far, normal);
  float light = max(0.05, dot(normal, normalize(float3(0.5, 1, 0.7))));
  switch (voxel_data.w) {
    case 0:
      return float4(1, 1, 1, 1);
    case 1:
      return float4(1, 1, 0, 1) * light;
    case 2:
      return float4(0.7, 0.7, 0, 1) * light;
    default:
      return float4(1, 0, 0, 1);
  }
}
