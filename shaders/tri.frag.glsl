#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
 vec2 uv = gl_FragCoord.xy / vec2(800.0, 600.0);
uv = uv * 2.0 - 1.0;

// Face circle
float face = length(uv) - 0.7;

// Eyes
vec2 left_eye = uv - vec2(-0.25, 0.2);
vec2 right_eye = uv - vec2(0.25, 0.2);
float eye1 = length(left_eye) - 0.08;
float eye2 = length(right_eye) - 0.08;

// Smile (arc)
float smile_y = uv.y + 0.2;
float smile_dist = sqrt(uv.x * uv.x + smile_y * smile_y);
float smile = abs(smile_dist - 0.35) - 0.03;

// Face conditions
bool is_face = face < 0.0 && eye1 > 0.0 && eye2 > 0.0 && (smile > 0.0 || uv.y > -0.1);
bool is_eye = eye1 < 0.0 || eye2 < 0.0;
bool is_smile = smile < 0.0 && uv.y < -0.1;

if (is_face) {
    vec3 color = vec3(1.0, 1.0, 0.3); // Yellow face
    outColor = vec4(color, 1.0);
} else if (is_eye || is_smile) {
    vec3 color = vec3(0.1, 0.1, 0.1); // Dark features
    outColor = vec4(color, 1.0);
} else {
    discard;
}





}
