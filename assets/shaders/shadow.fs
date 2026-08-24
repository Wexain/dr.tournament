#version 330

// Simple shadow fragment shader
// Blob shadow / simple shadow map lookup

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec3 lightDir;
uniform vec4 colDiffuse;

void main() {
    // Basic directional lighting
    vec3 normal = normalize(fragNormal);
    vec3 lightDirection = normalize(-lightDir);
    
    // Diffuse
    float diff = max(dot(normal, lightDirection), 0.0);
    
    // Ambient
    vec3 ambient = vec3(0.3, 0.3, 0.35);
    
    // Combine
    vec3 lighting = ambient + vec3(1.0, 0.98, 0.95) * diff;
    
    vec4 texColor = texture(texture0, fragTexCoord);
    finalColor = vec4(texColor.rgb * lighting * colDiffuse.rgb, texColor.a * colDiffuse.a);
}
