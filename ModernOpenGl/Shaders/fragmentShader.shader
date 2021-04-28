#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec3 Pos;

out vec4 FragColor;

uniform vec3 lightColor;
uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float strength;
uniform int shinyness;

void main()
{
    float ambientStrength = 0.1f;
    float specularStrength = strength;

    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);

    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shinyness);
    vec3 specular = specularStrength * spec * lightColor;

    FragColor = texture(texture1, vec2(Pos)) * vec4((ambient + diffuse + specular), 1.0f);
}