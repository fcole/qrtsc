/*****************************************************************************\

polygon_render.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Renders the polygons of the scene.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

// interpolated values
varying vec3			  vert_light_world;
varying vec3			  vert_normal_world;
varying vec3			  vert_pos_camera;
varying vec3			  vert_pos_world;
varying vec4			  vert_pos_clip;

uniform vec3 bsphere_center;
uniform float bsphere_radius;

void main()
{
    vec4 cam_bsphere_center = gl_ModelViewMatrix * vec4(bsphere_center, 1);
    float cam_far_z = cam_bsphere_center.z - bsphere_radius;
    float cam_near_z = cam_bsphere_center.z + bsphere_radius;

    float scaled_z = (vert_pos_camera.z - cam_near_z) /
                     (cam_far_z - cam_near_z);

    vec4 out_col = vec4(1.0 - scaled_z);

    out_col.a = 1.0;
				      
    gl_FragColor = out_col;
}
