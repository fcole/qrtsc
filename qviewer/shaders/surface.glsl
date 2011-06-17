uniform sampler3D texture;
uniform float texture_scale;
uniform float contrast;
uniform float scale_x;
uniform float scale_y;

uniform float use_texture;

varying vec3 vert_pos_world;

vec4 surfaceColor()
{
    vec3 coordinate = vert_pos_world * texture_scale;
    coordinate.x *= scale_x;
    coordinate.y *= scale_y;

    vec4 tex_col = texture3D(texture, coordinate);

    tex_col = ((tex_col - 0.5) * contrast) + 0.5;

    return tex_col * use_texture + gl_Color * (1.0 - use_texture);
}
