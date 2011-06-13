/*****************************************************************************\

shiny.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Renders the polygons of the scene.

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

// interpolated values
varying vec3 vert_light_world;
varying vec3 vert_normal_world;
varying vec3 vert_pos_camera;
varying vec3 vert_pos_world;
varying vec4 vert_pos_clip;
varying vec3 camera_pos_world;

void main()
{
	float diffuse = max(dot(vert_normal_world, vert_light_world),0.0);
    vec3 h = normalize(camera_pos_world + vert_light_world);
    float specular = dot(h, vert_normal_world);
    float shininess = 50;

	float shiny = pow(specular, shininess);
    vec4 specular_color = vec4(1,1,1,1);

    float kd = 0.6;
    float ks = 0.4;
	
	vec4 out_col = kd * diffuse * gl_Color + ks * shiny * specular_color;
    out_col.a = 1.0;
				      
    gl_FragColor = out_col;
}
