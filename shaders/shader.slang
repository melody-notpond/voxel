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
  return int3(floor(pos * chunk.voxel_count));
}

float next_voxel_plane(inout int3 voxel_pos, int3 step, inout float3 t_max,
  float3 t_delta, out float3 normal) {
  if (t_max.x < t_max.y) {
    if (t_max.x < t_max.z) {
      t_max.x += t_delta.x;
      voxel_pos.x += step.x;
      normal = float3(-step.x, 0, 0);
      return t_max.x - t_delta.x;
    } else {
      t_max.z += t_delta.z;
      voxel_pos.z += step.z;
      normal = float3(0, 0, -step.z);
      return t_max.z - t_delta.z;
    }
  } else {
    if (t_max.y < t_max.z) {
      t_max.y += t_delta.y;
      voxel_pos.y += step.y;
      normal = float3(0, -step.y, 0);
      return t_max.y - t_delta.y;
    } else {
      t_max.z += t_delta.z;
      voxel_pos.z += step.z;
      normal = float3(0, 0, -step.z);
      return t_max.z - t_delta.z;
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

static float EPSILON = 0.01;

// uses dda traversal
uint raymarch(float3 pos, float3 dir, uint limit, float start, float end,
  out float3 voxel_pos, out float3 normal, out float depth) {
  float t = start;
  float3 origin = pos + t * dir;
  int3 coord = pos_to_voxel_coords(origin);
  int3 step = sign(dir);
  float3 t_max = bound(origin, dir);
  float3 t_delta = step / dir / chunk.voxel_count;

  for (int i = 0; i < limit && t < end; i++) {
    t = next_voxel_plane(coord, step, t_max, t_delta, normal);
    uint voxel = voxels.Load(int4(coord, 0));
    if (voxel != 0) {
      voxel_pos = origin + (t - EPSILON) * dir;
      depth = (t - start) / (end - start);
      return voxel;
    }
  }

  voxel_pos = float3(0);
  normal = float3(0);
  depth = 1.;
  return 0;
}

struct frag_out {
  float4 color : SV_Target;
  float depth : SV_Depth;
}

[shader("fragment")]
frag_out frag_main(in float4 pos : SV_Position) {
  float2 uv = (2 * pos.xy - cam.viewport) / cam.viewport;

  float3 ray_origin = mul(mul(chunk.model_inv, cam.view_inv),
    float4(0, 0, 0, 1)).xyz;
  float4 ray_dir4 = mul(mul(chunk.model_inv, cam.proj_view_inv),
    float4(uv, 0, 1));
  float3 ray_dir = normalize(ray_dir4.xyz / ray_dir4.w - ray_origin);
  float3 normal;
  float3 voxel_pos;
  float depth;

  uint voxel = raymarch(ray_origin, ray_dir, cam.max_marches, cam.z_near,
    cam.z_far, voxel_pos, normal, depth);
  if (voxel == 0)
    return {float4(0), depth};

  float3 light_dir = normalize(float3(0.5, 1, 0.7));
  float3 temp;
  float temp_;
  uint cover = raymarch(voxel_pos, light_dir, cam.max_marches, EPSILON,
    cam.z_far, temp, temp, temp_);

  float light = 0.05;
  if (cover == 0)
    light = max(0.05, dot(normal, light_dir));
  switch (voxel) {
    case 0:
      return {float4(0), depth};
    case 1:
      return {float4(1, 1, 0, 1) * light, depth};
    case 2:
      return {float4(0.7, 0.7, 0, 1) * light, depth};
    default:
      return {float4(1, 0, 0, 1), depth};
  }
}
