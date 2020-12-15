#version 300 es

precision mediump float;

in vec3 aPosition;
in vec2 aTexCoord;

out vec2 vUV;

uniform mat4 uMvpMatrix;

void main()
{
  gl_Position = uMvpMatrix * vec4( aPosition, 1.0 );

  vUV = aTexCoord;
}
