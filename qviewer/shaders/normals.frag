/*****************************************************************************\

polygon_render.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Renders the polygons of the scene.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

// interpolated values
varying vec3			  vert_normal_camera;

void main()
{
    vec4 out_col = vec4(vert_normal_camera*0.5 + 0.5,1.0);
				      
    gl_FragColor = out_col;
}
