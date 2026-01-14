static float2 vertices[4] = {
  float2(-1.,  1.),
  float2( 1.,  1.),
  float2(-1., -1.),
  float2( 1., -1.)
};

[shader("vertex")]
float4 vert_main(uint id : SV_VertexID) : SV_Position {
  return float4(vertices[id], 0.0, 1.0);
}

float sd_sphere(float3 p, float3 centre, float radius) {
  return length(p - centre) - radius;
}

float sd_box(float3 p, float3 centre, float3 bounds) {
  p = p - centre;
  float3 q = abs(p) - bounds;
  return length(max(q, 0)) + min(max(q.x, max(q.y, q.z)), 0);
}

float sd_union(float p, float q) {
  return min(p, q);
}

float sunion(float p, float q, float k) {
  k *= 4.0;
  float h = max(k-abs(p-q),0.0);
  return min(p, q) - h*h*0.25/k;
}

float ssub(float p, float q, float k) {
  return -sunion(p, -q, k);
}

float sintersect(float p, float q, float k) {
  return -sunion(-p, -q, k);
}

float sd_sub(float p, float q) {
  return max(-p, q);
}

float sd_intersect(float p, float q) {
  return max(p, q);
}

float2x2 rot2d(float angle) {
  float s = sin(angle);
  float c = cos(angle);
  return float2x2(c, s, -s, c);
}

float sdf(float3 p) {
  float sphere = sd_sphere(p, float3(0, 1, -3), 1);
  float3 q = p;
  q.xz = mul(rot2d(3.14 / 4), q.xz);
  q.yz = mul(rot2d(3.14 / 4), q.yz);
  float box = sd_box(q, float3(0, 2, -3), float3(0.75));
  float ground = p.y + 0.75;
  return sd_union(ground, sunion(sphere, box, 1));
}

static const float EPSILON = 0.001;

struct Camera {
  float4x4 view;
  float2 viewport;
  float tan_fov;
  float z_near;
  float z_far;
  int max_marches;
}

ConstantBuffer<Camera> cam;

[shader("fragment")]
float4 frag_main(in float4 pos : SV_Position)
  : SV_Target {
  float2 uv = float2(1, -1) * (2 * pos.xy - cam.viewport) / cam.viewport.y;

  float3 ray_origin = float3(0);
  float3 ray_dir = float3(uv * cam.tan_fov, -1);
  ray_dir = normalize(ray_dir);
  float3 color;
  float t = cam.z_near;

  for (int i = 0; i < cam.max_marches && t < cam.z_far; i++) {
    float3 p = ray_origin + ray_dir * t;
    p = mul(cam.view, float4(p, 1)).xyz;
    float dist = sdf(p);
    if (dist < EPSILON)
      break;
    t += dist;
  }

  color = float3(t * 0.05);

  return float4(color, 1);
}
