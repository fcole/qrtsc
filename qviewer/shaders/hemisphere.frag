/*****************************************************************************\

hemisphere.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Renders the polygons of the scene.

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

// interpolated values
varying vec3			  vert_light_world;
varying vec3			  vert_normal_world;

void main()
{
	float diffuse = dot(vert_normal_world, vert_light_world);
    float z = diffuse * 0.5 + 0.5;
	
	vec4 out_col = z * surfaceColor();
    out_col.a = 1.0;
				      
    gl_FragColor = out_col;
}
