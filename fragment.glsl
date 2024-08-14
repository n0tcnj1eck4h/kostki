#version 460

layout(location=0) out vec4 color;

layout(std140, column_major, binding=0) uniform UniformBlock {
  vec3 camera_position;
  vec3 camera_direction;
  vec2 screen_size;
};

layout(std430, binding = 0) buffer StorageBlock
{
  uint storage_data[];
};

const vec3 UP = vec3(0,1,0);

const uint MAX_ITERATIONS = 1024;
const float MAX_DISTANCE = 1024.0;

void main() {
  const vec2 pixel_position = (gl_FragCoord.xy / screen_size) * 2 - vec2(1);
  const vec3 right = normalize(cross(camera_direction, UP));
  const vec3 up = normalize(cross(right, camera_direction));
  const vec3 ray_direction = camera_direction 
                             + right * pixel_position.x * screen_size.x / screen_size.y
                             + up * pixel_position.y;

  const vec3 S = vec3(
    length(ray_direction / ray_direction.x),
    length(ray_direction / ray_direction.y),
    length(ray_direction / ray_direction.z)
  );

  const ivec3 istep = ivec3(sign(ray_direction));
  ivec3 ipos = ivec3(floor(camera_position));

  vec3 ray_length_1d = abs(vec3(ipos + (istep + ivec3(1)) / 2) - camera_position) * S;

  float bloom = dot(camera_direction, ray_direction);

  float dist = 0;
  for(uint i = 0; i < MAX_ITERATIONS && dist < MAX_DISTANCE; i++){
    if(ray_length_1d.x < ray_length_1d.y && ray_length_1d.x < ray_length_1d.z) {
      ipos.x += istep.x;
      dist = ray_length_1d.x;
      ray_length_1d.x += S.x;
    }
    else if(ray_length_1d.y < ray_length_1d.z) {
      ipos.y += istep.y;
      dist = ray_length_1d.y;
      ray_length_1d.y += S.y;
    }
    else {
      ipos.z += istep.z;
      dist = ray_length_1d.z;
      ray_length_1d.z += S.z;
    }

    int array_index = (ipos.x + ipos.y * 16 + ipos.z * 16 * 16) % (16 * 16 * 16);
    uint block_data = storage_data[array_index];
    if(block_data != 0) {
      float r = float((block_data & 0x00FF0000) >> 16) / 256.0;
      float g = float((block_data & 0x0000FF00) >> 8 ) / 256.0;
      float b = float( block_data & 0x000000FF       ) / 256.0;

      color = vec4(r, g, b, 1) / dist * 10.0;
      return;
    }

  }

  color = vec4(vec3(0) * bloom,1);
}
