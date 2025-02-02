#version 330 core

#define POINT_LIGHT_COUNT 4

in vec2 Texcoord;
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 WorldBinormal;
in vec3 WorldPos;

out vec4 FragColor;

struct DirectionalLight
{
	vec3 lightDir;
	vec4 lightColor;
};

struct PointLight
{
	vec3 lightPos;
	vec4 lightColor;

	float constant;
	float linear;
	float quadratic;
};

struct SpotLight
{
	vec3 lightPos;
	vec4 lightColor;
	vec3 lightDir;

	float innerCutoff;
	float outerCutoff;

	float constant;
	float linear;
	float quadratic;
};

uniform DirectionalLight dirLight;
uniform PointLight pointLight[POINT_LIGHT_COUNT];
uniform SpotLight spotLight;

uniform float ambient;
uniform vec3 viewPos;
uniform mat4 lightview;
uniform mat4 lightprojection;

uniform sampler2D albedo;
uniform sampler2D normalmap;
uniform sampler2D metallicmap;
uniform sampler2D roughnessmap;
uniform sampler2D shadowmap;
uniform sampler2D brdfLUT;
uniform samplerCube prefilterMap;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 PBRLightingImp(vec3 albedo, vec3 radiance, vec3 V, vec3 N, vec3 L, float roughness, float metallic)
{
	// base reflectivity
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 H = normalize(V + L);
	float HdotV = max(dot(H, V), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = fresnelSchlick(HdotV, F0);

	vec3 ks = F;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - metallic;

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * NdotV * NdotL;
	vec3 specular = numerator / max(denominator, 0.001);

	vec3 tmp_color = (kd * albedo / PI + specular) * radiance * NdotL;

	return tmp_color;
}

vec3 PBRLighting(vec3 albedo, vec3 worldPos, vec3 V, vec3 N, float roughness, float metallic, float shadow)
{
	vec3 tmp_color = vec3(0.0);

	// directional light
	vec3 tmp_dirLight = normalize(-dirLight.lightDir.xyz);
	vec3 tmp_dirColor = dirLight.lightColor.rgb * 2.7;
	tmp_color += PBRLightingImp(albedo, tmp_dirColor, V, N, tmp_dirLight, roughness, metallic) * shadow;

	// point light
	for (int i = 0; i < POINT_LIGHT_COUNT; ++i)
	{
		vec3 tmp_pointOffset = pointLight[i].lightPos - worldPos;
		vec3 tmp_pointDir = normalize(tmp_pointOffset);
		float tmp_distance = length(tmp_pointOffset);
		// atten = 1 / (a*x*x + b*x + c)
		float tmp_attenuation = 1.0 / (pointLight[i].constant + pointLight[i].linear * tmp_distance + pointLight[i].quadratic * tmp_distance * tmp_distance);
		vec3 tmp_pointColor = pointLight[i].lightColor.rgb * tmp_attenuation;
		tmp_color += PBRLightingImp(albedo, tmp_pointColor, V, N, tmp_pointDir, roughness, metallic);
	}

	// spot light
	vec3 tmp_spotOffset = spotLight.lightPos - worldPos;
	vec3 tmp_spotDir = normalize(tmp_spotOffset);
	float tmp_spotDistance = length(tmp_spotOffset);
	// atten = 1 / (a*x*x + b*x + c)
	float tmp_spotAtten = 1.0 / (spotLight.constant + spotLight.linear * tmp_spotDistance + spotLight.quadratic * tmp_spotDistance * tmp_spotDistance);
	float tmp_cos = dot(tmp_spotDir, normalize(-spotLight.lightDir)); 
    float tmp_epsilon = spotLight.innerCutoff - spotLight.outerCutoff;
    float tmp_intensity = clamp((tmp_cos - spotLight.outerCutoff) / tmp_epsilon, 0.0, 1.0);
    vec3 tmp_spotColor = spotLight.lightColor.rgb * tmp_spotAtten * tmp_intensity;
    tmp_color += PBRLightingImp(albedo, tmp_spotColor, V, N, tmp_spotDir, roughness, metallic);

	return tmp_color;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 EnvBRDFApprox(vec3 SpecularColor, float Roughness, float NoV)
{
	// [ Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II" ]
	// Adaptation to fit our G term.
	vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
	vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
	vec4 r = Roughness * c0 + c1;
	float a004 = min( r.x * r.x, exp2( -9.28 * NoV ) ) * r.x + r.y;
	vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;

	// Anything less than 2% is physically impossible and is instead considered to be shadowing
	// Note: this is needed for the 'specular' show flag to work, since it uses a SpecularColor of 0
	AB.y *= min(max( 50.0 * SpecularColor.g, 0.0 ), 1.0);

	return SpecularColor * AB.x + AB.y;
}

vec3 IBLLighting(vec3 albedo, vec3 V, vec3 N, float roughness, float metallic)
{
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 ks = F;
	vec3 kd = 1.0 - ks;
	kd *= 1.0 - metallic;

	vec3 irradiance = texture(prefilterMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	// const float MAX_REFLECTION_LOD = 4.0;
	// vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
	vec3 R = reflect(-V, N);
	vec3 prefilteredColor = texture(prefilterMap, R).rgb;   
	vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
	vec3 approx = EnvBRDFApprox(F, roughness, max(dot(N, V), 0.0));
	specular = prefilteredColor * approx;
	
	float ao = 1.0;
	vec3 ambient = (kd * diffuse + specular) * ao;

	return ambient;
}

// ACES Filmic Tone Mapping Curve
//
// Adapted from code by Krzysztof Narkowicz
// https://knarkowicz.wordpress.com/2016/01/06/
// aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float tA = 2.51f;
    float tB = 0.03f;
    float tC = 2.43f;
    float tD = 0.59f;
    float tE = 0.14f;
    return clamp((x * (tA * x + tB)) / (x * (tC * x + tD) + tE), 0.0, 1.0);
}

void main()
{
	vec3 tmp_normal = normalize(WorldNormal);
	vec3 tmp_tangent = normalize(WorldTangent);
	vec3 tmp_binormal = normalize(WorldBinormal);
	vec3 tmp_viewDir = normalize(viewPos.xyz - WorldPos);

	vec3 tmp_albedo = pow(texture(albedo, Texcoord).xyz, vec3(2.2));
	vec3 tmp_normaldir = texture(normalmap, Texcoord).xyz;
	tmp_normaldir = normalize(tmp_normaldir * 2.0 - 1.0);
	tmp_normaldir = normalize(mat3(tmp_tangent, tmp_binormal, tmp_normal) * tmp_normaldir);
	float tmp_metallic = texture(metallicmap, Texcoord).r;
	float tmp_roughness = texture(roughnessmap, Texcoord).r;

	// shadow
	vec4 tmp_lightClipPos = lightprojection * lightview * vec4(WorldPos, 1.0);
	vec4 tmp_lightScreenPos = (tmp_lightClipPos.xyzw / tmp_lightClipPos.w + 1.0) / 2.0;
	float tmp_depth = texture(shadowmap, tmp_lightScreenPos.xy).x + 0.01;
	float tmp_shadow = 1.0 - step(tmp_lightScreenPos.z, 1.0) * step(tmp_depth, tmp_lightScreenPos.z);

	vec3 tmp_pbrColor = PBRLighting(tmp_albedo, WorldPos, tmp_viewDir, tmp_normaldir, tmp_roughness, tmp_metallic, tmp_shadow);
	vec3 tmp_iblColor = IBLLighting(tmp_albedo, tmp_viewDir, tmp_normaldir, tmp_roughness, tmp_metallic);
	vec3 tmp_result = tmp_pbrColor + tmp_iblColor * ambient;
	
	FragColor = vec4(tmp_result, 1.0f);
}
