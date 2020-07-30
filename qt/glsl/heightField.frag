#version 330
in vec3 pos;
in vec2 tex;
in vec3 normal;
in vec3 fcolor;
out vec4 outcolor;
uniform vec3 lightPos;
uniform sampler2D texsampler;
void main()
{
    vec3 L = lightPos - pos;
	float lightDist = length(L);
	L = (1.0/lightDist) * L;
	vec4 Color = texture(texsampler, tex);
	vec3 N = normalize(normal);
	float diffuse = max(0.0, dot(L, N));
	vec3 V = -normalize(pos);
	vec3 H = L + V;
	H = normalize(H);
	float alpha=4.0;
	float specular = max(0.0,dot(N,H));
	specular = 0.5* pow(specular, alpha);
    vec4 c = (specular + diffuse  )* vec4(fcolor, 1.0) ;
    outcolor = c;
}
