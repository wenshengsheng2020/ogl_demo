#version 410

in vec4 Position;
out vec3 vPosition;

void main()
{
    vPosition = Position.xyz;
}
