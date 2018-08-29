#version 330
#extension GL_ARB_explicit_attrib_location : require
uniform mat4 MVP;
uniform mat3 MVIT;
uniform vec2 dx;
layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vColor;
smooth out vec3 pos;
smooth out vec2 tex;
smooth out vec3 normal;
smooth out vec3 fcolor;
uniform sampler2D texsampler;
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
void main()
{   
	
	//normal = normalize(vNormal);
	tex = vPos.xz;
	tex.y = 1.0f-tex.y;
	vec4 Color = texture(texsampler, tex);
	vec4 d = vec4(vPos, 1.0f);
	d.y = float(Color.x);
	vec3 hsv = vec3(float(Color.x), 0.7f, 0.7f);
	float hp =0;
	float hm = 0;
	hp = texture(texsampler, tex + vec2( dx.x, 0)).x;
	hm = texture(texsampler, tex - vec2( dx.x, 0)).x;
	float dhdx = (hp-hm)/5;
	hp = texture(texsampler, tex + vec2(0, dx.x)).x;
	hm = texture(texsampler, tex - vec2(0, dx.x)).x;
	float dhdy = (hp-hm)/5;
	normal = normalize(vec3(-dhdx, 2.0f * dx.x, -dhdy));
	vec4 p = MVP * d;
	fcolor = vColor + hsv2rgb(hsv);
	gl_Position = p;
	pos = vPos;
}
