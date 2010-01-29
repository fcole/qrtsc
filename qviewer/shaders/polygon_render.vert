/*****************************************************************************\

standard.vert
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A basic vertex shader used with multiple programs.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec3 light_dir_world;

varying vec3 vert_pos_world;
varying vec3 vert_pos_camera;
varying vec4 vert_pos_clip;
varying vec3 vert_light_world;
varying vec3 vert_normal_camera;
varying vec3 vert_normal_world;
varying vec3 camera_pos_world;

void main()
{
    gl_Position = ftransform();
    gl_FrontColor = gl_BackColor = gl_Color;
    gl_TexCoord[0] = gl_MultiTexCoord0;
	
    vert_pos_clip = gl_Position;
    vert_pos_world = gl_Vertex.xyz;
    vert_pos_camera = (gl_ModelViewMatrix * gl_Vertex).xyz;
    
    camera_pos_world = (gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	
    vert_light_world = normalize(light_dir_world);
	
    vert_normal_world = normalize(gl_Normal);
    vert_normal_camera = (gl_ModelViewMatrixInverseTranspose * vec4(vert_normal_world, 0)).xyz;
	
    // hack to handle backfacing polygons in bad models
    /*float dotcamerasign = sign(dot(vert_normal_camera, vert_vec_view));
    vert_normal_camera *= dotcamerasign;
    vert_normal_world_ff = vert_normal_world * dotcamerasign;*/
}
