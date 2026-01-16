static float3 vertices[8] = {
  float3(0.5, 0.5, -0.5),
  float3(0.5, -0.5, -0.5),
  float3(0.5, 0.5, 0.5),
  float3(0.5, -0.5, 0.5),
  float3(-0.5, 0.5, -0.5),
  float3(-0.5, -0.5, -0.5),
  float3(-0.5, 0.5, 0.5),
  float3(-0.5, -0.5, 0.5),
};

static uint indices[] = {
  4, 2, 0,
  2, 7, 3,
  6, 5, 7,
  1, 7, 5,
  0, 3, 1,
  4, 1, 5,
  4, 6, 2,
  2, 6, 7,
  6, 4, 5,
  1, 3, 7,
  0, 2, 3,
  4, 0, 1
};

struct Camera {
  float4x4 proj_view;
  float4x4 view_inv;
  float4x4 proj_view_inv;
  float2 viewport;
  float tan_fov;
  float z_near;
  float z_far;
  int max_marches;
}

struct Chunk {
  float4x4 model;
}

ConstantBuffer<Camera> cam;
ConstantBuffer<Chunk> chunk;
Texture3D voxels;

[shader("vertex")]
float4 vert_main(uint id : SV_VertexID) : SV_Position {
  float4 p = float4(vertices[indices[id]], 1.0);
  return mul(cam.proj_view, mul(chunk.model, p));
}

float4 sd_sphere(float3 p, float3 centre, float radius) {
  return float4(1, 1, 0, length(p - centre) - radius);
}

float4 sd_box(float3 p, float3 centre, float3 bounds) {
  p = p - centre;
  float3 q = abs(p) - bounds;
  return float4(0, 0, 1, length(max(q, 0)) + min(max(q.x, max(q.y, q.z)), 0));
}

float4 sd_union(float4 p, float4 q) {
  if (p.a < q.a)
    return p;
  else return q;
}

float4 sd_intersect(float4 p, float4 q) {
  if (p.a > q.a)
    return p;
  else return q;
}

float4 sd_sub(float4 p, float4 q) {
  return sd_intersect(-p, q);
}

float4 sunion(float4 p, float4 q, float k) {
  k *= 4.0;
  float4 h = sd_intersect(k-abs(p-q),0.0);
  return sd_union(p, q) - h*h*0.25/k;
}

float4 ssub(float4 p, float4 q, float k) {
  return -sunion(p, -q, k);
}

float4 sintersect(float4 p, float4 q, float k) {
  if (p.a > q.a)
    return p;
  else return q;
}

float2x2 rot2d(float angle) {
  float s = sin(angle);
  float c = cos(angle);
  return float2x2(c, s, -s, c);
}

float4 sdf(float3 p) {
  return sd_sphere(p, float3(0, 0, 0), 0.5);
  // float4 sphere = sd_sphere(p, float3(0, 1, -3), 1);
  // float3 q = p;
  // q.xz = mul(rot2d(3.14 / 4), q.xz);
  // q.yz = mul(rot2d(3.14 / 4), q.yz);
  // float4 box = sd_box(q, float3(0, 2, -3), float3(0.75));
  // float4 ground = float4(0, 1, 0, p.y + 0.75);
  // return sd_union(ground, sunion(sphere, box, 1));
}

static const float EPSILON = 0.001;

float3 calculate_normal(float3 p) {
  float3 v1 = float3(
    sdf(p + float3(EPSILON, 0, 0)).a,
    sdf(p + float3(0, EPSILON, 0)).a,
    sdf(p + float3(0, 0, EPSILON)).a);
  float3 v2 = float3(
    sdf(p - float3(EPSILON, 0, 0)).a,
    sdf(p - float3(0, EPSILON, 0)).a,
    sdf(p - float3(0, 0, EPSILON)).a);
  return normalize(v1 - v2);
}

[shader("fragment")]
float4 frag_main(in float4 pos : SV_Position)
  : SV_Target {
  float2 uv = (2 * pos.xy - cam.viewport) / cam.viewport;

  float3 ray_origin = mul(cam.view_inv, float4(0, 0, 0, 1)).xyz;
  float4 ray_dir4 = mul(cam.proj_view_inv, float4(uv, 0, 1));
  float3 ray_dir = normalize(ray_dir4.xyz / ray_dir4.w - ray_origin);
  float3 color = float3(0, 0.3, 0.8);
  float t = cam.z_near;
  float light = 0;

  for (int i = 0; i < cam.max_marches && t < cam.z_far; i++) {
    float3 p = ray_origin + ray_dir * t;
    float4 d = sdf(p);
    if (d.a < EPSILON) {
      light = max(0.05, dot(calculate_normal(p), normalize(float3(1, 1, 1))));
      color = d.rgb;
      break;
    }
    t += d.a;
  }

  float depth_mul = (cam.z_far - t) / (cam.z_far - cam.z_near);
  color *= max(0.01, depth_mul * light);

  return float4(color, 1);
}
