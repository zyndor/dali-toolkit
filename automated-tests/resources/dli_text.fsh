#version 300 es

precision mediump float;

uniform sampler2D sAlbedo;
uniform vec4 uColor;

in vec2 vUV;

out vec4 FragColor;

void main()
{
  vec4 color = texture( sAlbedo, vUV );
  FragColor = vec4( color.rgb, uColor.a * color.a );
}
