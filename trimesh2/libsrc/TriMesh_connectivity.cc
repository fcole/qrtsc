/*
Szymon Rusinkiewicz
Princeton University

TriMesh_connectivity.cc
Manipulate data structures that describe connectivity between faces and verts.

Modified by Forrester Cole (fcole@cs.princeton.edu) 
to maintain CCW ordering of adjacent faces.
*/


#include <stdio.h>
#include "TriMesh.h"
#include <algorithm>
#include <map>
using std::find;


// Find the direct neighbors of each vertex
void TriMesh::need_neighbors()
{
	if (!neighbors.empty())
		return;

	need_faces();
	if (faces.empty())
		return;

	dprintf("Finding vertex neighbors... ");
	int nv = vertices.size(), nf = faces.size();

	vector<int> numneighbors(nv);
	for (int i = 0; i < nf; i++) {
		numneighbors[faces[i][0]]++;
		numneighbors[faces[i][1]]++;
		numneighbors[faces[i][2]]++;
	}

	neighbors.resize(nv);
	for (int i = 0; i < nv; i++)
		neighbors[i].reserve(numneighbors[i]+2); // Slop for boundaries

#if 0
	for (int i = 0; i < nf; i++) {
		for (int j = 0; j < 3; j++) {
			vector<int> &me = neighbors[faces[i][j]];
			int n1 = faces[i][(j+1)%3];
			int n2 = faces[i][(j+2)%3];
			if (find(me.begin(), me.end(), n1) == me.end())
				me.push_back(n1);
			if (find(me.begin(), me.end(), n2) == me.end())
				me.push_back(n2);
		}
	}
#else
	
	vector<std::map<int, int> > prev(nv), next(nv);
	
	for (int i = 0; i < nf; i++) {
		for (int j = 0; j < 3; j++) {
			int n0 = faces[i][j];
			int n1 = faces[i][(j+1)%3];
			int n2 = faces[i][(j+2)%3];
			
			prev[n0][n2] = n1;
			next[n0][n1] = n2;
		}
	}
	
	for(int i = 0; i < nv; ++i) {
		if(prev[i].empty())
			continue;
		int startVertex = prev[i].begin()->second;
		
		int count = 0;
		while(prev[i].find(startVertex) != prev[i].end() && count++ < numneighbors[i]) {
			startVertex = prev[i][startVertex];
		}
		
		int curVertex = startVertex;
		
		bool haveNextVertex = true;
		
		do {
			neighbors[i].push_back(curVertex);
			
			haveNextVertex = (next[i].find(curVertex) != next[i].end());
			if(haveNextVertex)
				curVertex = next[i][curVertex];
			
		} while(haveNextVertex && curVertex != startVertex);
	}
			
#endif
	
	dprintf("Done.\n");
}

int next_face(int v_center, int f, TriMesh* mesh)
{
    TriMesh::Face& face = mesh->faces[f];
    int i_center = (face[0] == v_center) ? 0 : ((face[1] == v_center) ? 1 : 2);
    int i_next = (i_center + 1) % 3;
    return mesh->across_edge[f][i_next];
}

int prev_face(int v_center, int f, TriMesh* mesh)
{
    TriMesh::Face& face = mesh->faces[f];
    int i_center = (face[0] == v_center) ? 0 : ((face[1] == v_center) ? 1 : 2);
    int i_prev = (i_center - 1 + 3) % 3;
    return mesh->across_edge[f][i_prev];
}

// Find the faces touching each vertex
void TriMesh::need_adjacentfaces()
{
	if (!adjacentfaces.empty())
		return;

	need_faces();
	if (faces.empty())
		return;

	dprintf("Finding vertex to triangle maps... ");
	int nv = vertices.size(), nf = faces.size();

	vector<int> numadjacentfaces(nv);
	for (int i = 0; i < nf; i++) {
		numadjacentfaces[faces[i][0]]++;
		numadjacentfaces[faces[i][1]]++;
		numadjacentfaces[faces[i][2]]++;
	}

	adjacentfaces.resize(vertices.size());
	for (int i = 0; i < nv; i++)
		adjacentfaces[i].reserve(numadjacentfaces[i]);

	for (int i = 0; i < nf; i++) {
		for (int j = 0; j < 3; j++) {
			adjacentfaces[faces[i][j]].push_back(i);
        }
	}

    need_across_edge();

	adjacentfaces.resize(vertices.size());
    for (int i = 0; i < nv; i++) {
		adjacentfaces[i].reserve(numadjacentfaces[i]);

        // Rewind to the beginning of this ring.
        int f = adjacentfaces[i][0];
        int f_prev = prev_face(i,f,this);
        while (f_prev >= 0 && f_prev != adjacentfaces[i][0]) {
			f = f_prev;
            f_prev = prev_face(i,f,this);
        }
        // Walk the ring and add faces in order.
		int counter = 0;
		int f_start = f;
		do {
            adjacentfaces[i][counter++] = f;
            f = next_face(i,f,this);
		} while (f >= 0 && f != f_start);
    }

	dprintf("Done.\n");
}


// Find the face across each edge from each other face (-1 on boundary)
// If topology is bad, not necessarily what one would expect...
void TriMesh::need_across_edge()
{
	if (!across_edge.empty())
		return;

	need_adjacentfaces();
	if (adjacentfaces.empty())
		return;

	dprintf("Finding across-edge maps... ");

	int nf = faces.size();
	across_edge.resize(nf, Face(-1,-1,-1));

	for (int i = 0; i < nf; i++) {
		for (int j = 0; j < 3; j++) {
			if (across_edge[i][j] != -1)
				continue;
			int v1 = faces[i][(j+1)%3];
			int v2 = faces[i][(j+2)%3];
			const vector<int> &a1 = adjacentfaces[v1];
			const vector<int> &a2 = adjacentfaces[v2];
			for (int k1 = 0; k1 < (int)(a1.size()); k1++) {
				int other = a1[k1];
				if (other == i)
					continue;
				vector<int>::const_iterator it =
					find(a2.begin(), a2.end(), other);
				if (it == a2.end())
					continue;
				int ind = (faces[other].indexof(v1)+1)%3;
				if (faces[other][(ind+1)%3] != v2)
					continue;
				across_edge[i][j] = other;
				across_edge[other][ind] = i;
				break;
			}
		}
	}

	dprintf("Done.\n");
}

