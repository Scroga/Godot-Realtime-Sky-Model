shader_type sky;
render_mode use_debanding;

uniform sampler2D skyTexture : source_color;

group_uniforms general;
uniform float rotation_y = 0.0;
uniform bool flip_x = false;
uniform bool flip_y = false;

void sky() {
	vec3 final_color = vec3(0.0);
	vec3 dir = normalize(EYEDIR);
	float s = sin(rotation_y);
	float c = cos(rotation_y);
	dir = vec3(
		c * dir.x - s * dir.z,
		dir.y,
		s * dir.x + c * dir.z
	);

	if (dir.y > 0.0) {
		vec2 disk_pos = dir.xz / (1.0 + dir.y);

		if (flip_x) {
			disk_pos.x = -disk_pos.x;
		}
		if (flip_y) {
			disk_pos.y = -disk_pos.y;
		}

		vec2 uv = vec2(
			0.5 + 0.5 * disk_pos.x,
			0.5 - 0.5 * disk_pos.y
		);

		if (distance(uv, vec2(0.5)) <= 0.5) {
			final_color = texture(skyTexture, uv).rgb;
		}
	}

	COLOR = final_color;
}