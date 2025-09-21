// Alternative: Use heart equation in fragment shader
#version 450
layout(location = 0) out vec3 fragColor;
void main()
{
    // Simple triangle to cover screen area
    vec2 triangle[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2(3.0, -1.0), 
        vec2(-1.0, 3.0)
    );
    
    gl_Position = vec4(triangle[gl_VertexIndex], 0.0, 1.0);
    fragColor = vec3(1.0, 0.4, 0.6); // Pink color
}
