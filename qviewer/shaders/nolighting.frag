/*****************************************************************************\

nolighting.frag
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
	vec4 out_col = surfaceColor();
    out_col.a = 1.0;
				      
    gl_FragColor = out_col;
}
