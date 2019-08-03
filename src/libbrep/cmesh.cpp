// This evolved from the original trimesh halfedge data structure code:

// Author: Yotam Gingold <yotam (strudel) yotamgingold.com>
// License: Public Domain.  (I, Yotam Gingold, the author, release this code into the public domain.)
// GitHub: https://github.com/yig/halfedge
//
// It ended up rather different, as we are concerned with somewhat different
// mesh properties for CDT and the halfedge data structure didn't end up being
// a good fit, but as it's derived from that source the public domain license
// is maintained for the changes as well.

#include "common.h"

#include "bu/color.h"
//#include "bn/mat.h" /* bn_vec_perp */
#include "bn/plot3.h"
#include "./cmesh.h"

// needed for implementation
#include <iostream>
#include <queue>

namespace cmesh
{

bool
cmesh_t::tri_add(triangle_t &tri, int check)
{

    // Skip degenerate triangles, but report true since this was never
    // a valid triangle in the first place
    if (tri.v[0] == tri.v[1] || tri.v[1] == tri.v[2] || tri.v[2] == tri.v[0]) {
	return true;
    }

    // Skip duplicate triangles, but report true as this triangle is in the mesh
    // already
    if (this->tris.find(tri) != this->tris.end()) {
	return true;
    }


    if (check) {
	std::vector<long> sml = boundary_loops(0)[0];
	std::cout << "loop v cnt: " << sml.size() << "\n";
    }

    // Add the triangle
    this->tris.insert(tri);

    std::cout << "tris cnt: " << tris.size() << "\n";

    // Populate maps
    long i = tri.v[0];
    long j = tri.v[1];
    long k = tri.v[2];
    this->v2tris[i].insert(tri);
    this->v2tris[j].insert(tri);
    this->v2tris[k].insert(tri);
    struct edge_t e[3];
    e[0].set(i, j);
    e[1].set(j, k);
    e[2].set(k, i);
    for (int ind = 0; ind < 3; ind++) {
	this->edges2tris[e[ind]] = tri;
	this->v2edges[e[ind].v[0]].insert(e[ind]);
    }

    // Update unordered edge triangle count
    struct uedge_t ue[3];
    for (int ind = 0; ind < 3; ind++) {
	ue[ind].set(e[ind].v[0], e[ind].v[1]);
	this->uedges2tris[ue[ind]].insert(tri);
    }

    // Now that we know the triangle count for the unordered edges: the
    // addition of a triangle may change the boundary edge set.  Update.
    for (int ind = 0; ind < 3; ind++) {
	if (this->uedges2tris[ue[ind]].size() == 1) {
	    this->current_bedges.insert(ue[ind]);
	} else {
	    this->current_bedges.erase(ue[ind]);
	}
    }

    // With the addition of the new triangle, we need to update the
    // point->edge mappings as well.
    for (int ind = 0; ind < 3; ind++) {
	// For the directed edge associated with the new triangle,
	// we may need to add
	if (this->current_bedges.find(ue[ind]) != this->current_bedges.end()) {
	    this->edge_pnt_edges[e[ind].v[0]].insert(e[ind]);
	    this->edge_pnt_edges[e[ind].v[1]].insert(e[ind]);
	}
	// For the directed edge that now (potentially) has a mate,
	// we need to remove
	struct edge_t oe(e[ind].v[1],e[ind].v[0]);
	if (this->current_bedges.find(ue[ind]) == this->current_bedges.end()) {
	    this->edge_pnt_edges[oe.v[0]].erase(oe);
	    this->edge_pnt_edges[oe.v[1]].erase(oe);
	}
    }

    // If we're growing a triangle set by marching, rather than dumping a bag
    // of triangles into the container, check to see if the triangle just added
    // causes a problem in the boundary loop.  If it does, reject it.
#if 0
    if (check) {
	for (int ind = 0; ind < 3; ind++) {
	    long v = tri.v[ind];
	    if (this->edge_pnt_edges[v].size() > 2) {
		std::cerr << "Triangle introduced a problem in boundary loop, cannot add\n";
		this->tri_remove(tri);
	    }
	}
    }
#endif

#if 0
    std::map<uedge_t, std::set<triangle_t>>::iterator uet_it;
    for (uet_it = uedges2tris.begin(); uet_it != uedges2tris.end(); uet_it++) {
	std::cerr << "edge " << (*uet_it).first.v[0] << "-" << (*uet_it).first.v[1] <<  " tri cnt: " << (*uet_it).second.size() << "\n";
    }
#endif

    return true;
}


void cmesh_t::tri_remove(triangle_t &tri)
{
    // Update edge maps
    long i = tri.v[0];
    long j = tri.v[1];
    long k = tri.v[2];
    struct edge_t e1(i, j);
    struct edge_t e2(j, k);
    struct edge_t e3(k, i);

    this->v2edges[i].erase(e1);
    this->v2edges[j].erase(e2);
    this->v2edges[k].erase(e3);

    this->v2tris[i].erase(tri);
    this->v2tris[j].erase(tri);
    this->v2tris[k].erase(tri);

    this->edges2tris.erase(e1);
    this->edges2tris.erase(e2);
    this->edges2tris.erase(e3);

    this->uedges2tris[uedge_t(e1)].erase(tri);
    this->uedges2tris[uedge_t(e2)].erase(tri);
    this->uedges2tris[uedge_t(e3)].erase(tri);

    // Remove the triangle
    this->tris.erase(tri);
}

std::vector<triangle_t>
cmesh_t::face_neighbors(const triangle_t &f)
{
    std::vector<triangle_t> result;
    long i = f.v[0];
    long j = f.v[1];
    long k = f.v[2];
    struct uedge_t e[3];
    e[0].set(i, j);
    e[1].set(j, k);
    e[2].set(k, i);
    for (int ind = 0; ind < 3; ind++) {
	std::set<triangle_t> faces = this->uedges2tris[e[ind]];
	std::set<triangle_t>::iterator f_it;
	for (f_it = faces.begin(); f_it != faces.end(); f_it++) {
	    if (*f_it != f) {
		result.push_back(*f_it);
	    }
	}
    }

    return result;
}


std::vector<triangle_t>
cmesh_t::vertex_face_neighbors(long vind)
{
    std::vector<triangle_t> result;
    std::set<triangle_t> faces = this->v2tris[vind];
    std::set<triangle_t>::iterator f_it;
    for (f_it = faces.begin(); f_it != faces.end(); f_it++) {
	result.push_back(*f_it);
    }
    return result;
}

std::set<uedge_t>
cmesh_t::boundary_edges(int use_brep_data)
{
    this->problem_edges.clear();
    std::set<uedge_t> result;
    std::map<uedge_t, std::set<triangle_t>>::iterator ue_it;
    for (ue_it = this->uedges2tris.begin(); ue_it != this->uedges2tris.end(); ue_it++) {
	if ((*ue_it).second.size() == 1) {
	    struct uedge_t ue((*ue_it).first);
	    int skip_pnt = 0;
	    if (use_brep_data && this->edge_pnts) {
		// If we have extra information from the Brep, we can filter out
		// some "bad" edges
		for (int ind = 0; ind < 2; ind++) {
		    ON_3dPoint *p = pnts[ue.v[ind]];
		    if (edge_pnts->find(p) == edge_pnts->end()) {
			// Strange edge count on a vertex not known
			// to be a Brep edge point - not a boundary
			// edge
			skip_pnt = 1;
		    }
		}
	    }

	    if (!skip_pnt) {
		result.insert((*ue_it).first);
	    } else {
		// Track these edges, as they represent places where subsequent
		// mesh operations will require extra care
		this->problem_edges.insert((*ue_it).first);
	    }
	}
    }
    return result;
}

edge_t
cmesh_t::find_boundary_oriented_edge(uedge_t &ue)
{
    // For the unordered boundary edge, there is exactly one
    // directional edge - find it
    std::set<edge_t>::iterator e_it;
    for (int i = 0; i < 2; i++) {
	std::set<edge_t> vedges = this->v2edges[ue.v[i]];
	for (e_it = vedges.begin(); e_it != vedges.end(); e_it++) {
	    uedge_t vue(*e_it);
	    if (vue == ue) {
		edge_t de(*e_it);
		return de;
	    }
	}
    }

    // This shouldn't happen
    edge_t empty;
    return empty;
}

std::vector<std::vector<long>>
cmesh_t::boundary_loops(int use_brep_data)
{
    std::set<edge_t>::iterator e_it;
    std::vector<std::vector<long>> results;

    std::set<uedge_t> bedges = this->boundary_edges(use_brep_data);

    if (!bedges.size()) {
	std::cerr << "No boundary edges in mesh??\n";
	return results;
    }

    std::set<uedge_t> unadded(bedges.begin(), bedges.end());
    std::vector<long> *wl = new std::vector<long>;

    uedge_t fedge = *(unadded.begin());
    unadded.erase(fedge);
    edge_t dedge = this->find_boundary_oriented_edge(fedge);

    long first_v = dedge.v[0];
    long prev_v = dedge.v[0];
    long curr_v = dedge.v[1];

    wl->push_back(first_v);
    wl->push_back(curr_v);

    while (unadded.size()) {
	std::set<edge_t> candidates;
	std::set<edge_t> vedges = this->v2edges[curr_v];
	uedge_t cue(prev_v, curr_v);
	for (e_it = vedges.begin(); e_it != vedges.end(); e_it++) {
	    uedge_t vue(*e_it);
	    if (cue == vue) continue;
	    if (bedges.find(vue) == bedges.end()) continue;
	    if (unadded.find(vue) == unadded.end()) continue;
	    candidates.insert(*e_it);
	    break;
	}
	if (candidates.size() != 1) {
	    std::cerr << "No viable candidate boundary edge??\n";
	    results.clear();
	    return results;
	}
	edge_t nedge = *(candidates.begin());
	prev_v = nedge.v[0];
	curr_v = nedge.v[1];
	wl->push_back(curr_v);
	uedge_t nuedge(nedge);
	unadded.erase(nuedge);
	if (curr_v == first_v) {
	    results.push_back(*wl);
	    delete wl;
	    if (unadded.size()) {
		wl = new std::vector<long>;

		fedge = *(unadded.begin());
		unadded.erase(fedge);
		dedge = this->find_boundary_oriented_edge(fedge);

		first_v = dedge.v[0];
		prev_v = dedge.v[0];
		curr_v = dedge.v[1];

		wl->push_back(first_v);
		wl->push_back(curr_v);
	    }
	}
    }
    if (curr_v != first_v) {
	std::cerr << "Unable to close loop!\n";
	results.clear();
	return results;
    }

    // TODO If we have more than one loop, need to determine which is the outer
    // loop.  A difficult problem in general, but CDT projections we should be able
    // to use the bbox of the 3D points to find the largest box
    if (results.size() > 1) {
    }

    return results;
}



std::set<long>
cmesh_t::interior_points(int use_brep_data)
{
    std::set<long> results;
    std::set<long> bedge_pnts;
    std::set<uedge_t> bedges = this->boundary_edges(use_brep_data);
    std::set<uedge_t>::iterator ue_it;
    for (ue_it = bedges.begin(); ue_it != bedges.end(); ue_it++) {
	bedge_pnts.insert((*ue_it).v[0]);
	bedge_pnts.insert((*ue_it).v[1]);
    }
    std::set<triangle_t>::iterator tr_it;
    for (tr_it = this->tris.begin(); tr_it != this->tris.end(); tr_it++) {
	for (int j = 0; j < 3; j++) {
	    long vind = (*tr_it).v[j];
	    if (bedge_pnts.find(vind) == bedge_pnts.end()) {
		results.insert(vind);
	    }
	}
    }

    return results;
}

std::vector<triangle_t>
cmesh_t::interior_incorrect_normals(int use_brep_data)
{
    std::vector<triangle_t> results;
    std::set<long> bedge_pnts;
    std::set<uedge_t> bedges = this->boundary_edges(use_brep_data);
    std::set<uedge_t>::iterator ue_it;
    for (ue_it = bedges.begin(); ue_it != bedges.end(); ue_it++) {
	bedge_pnts.insert((*ue_it).v[0]);
	bedge_pnts.insert((*ue_it).v[1]);
    }

    std::set<triangle_t>::iterator tr_it;
    for (tr_it = this->tris.begin(); tr_it != this->tris.end(); tr_it++) {
	ON_3dVector tdir = this->tnorm(*tr_it);
	ON_3dVector bdir = this->bnorm(*tr_it);
	if (tdir.Length() > 0 && bdir.Length() > 0 && ON_DotProduct(tdir, bdir) < 0.1) {
	    int epnt_cnt = 0;
	    for (int i = 0; i < 3; i++) {
		ON_3dPoint *p = pnts[(*tr_it).v[i]];
		epnt_cnt = (edge_pnts->find(p) == edge_pnts->end()) ? epnt_cnt : epnt_cnt + 1;
	    }
	    if (epnt_cnt == 2) {
		std::cerr << "UNCULLED problem point from surface???????\n";
		for (int i = 0; i < 3; i++) {
		    ON_3dPoint *p = pnts[(*tr_it).v[i]];
		    if (edge_pnts->find(p) == edge_pnts->end()) {
			std::cout << p->x << " " << p->y << " " << p->z << "\n";
		    }
		}
		results.clear();
		return results;
	    }
	    results.push_back(*tr_it);
	}
    }

    return results;
}


std::vector<triangle_t>
cmesh_t::singularity_triangles()
{
    std::vector<triangle_t> results;
    std::set<triangle_t> uniq_tris;
    std::set<long>::iterator s_it;

    for (s_it = sv.begin(); s_it != sv.end(); s_it++) {
	std::vector<triangle_t> faces = this->vertex_face_neighbors(*s_it);
	uniq_tris.insert(faces.begin(), faces.end());
    }

    results.assign(uniq_tris.begin(), uniq_tris.end());
    return results;
}

void
cmesh_t::set_brep_data(
	bool brev,
       	std::set<ON_3dPoint *> *e,
       	std::set<ON_3dPoint *> *s,
       	std::map<ON_3dPoint *, ON_3dPoint *> *n)
{
    this->m_bRev = brev;
    this->edge_pnts = e;
    this->singularities = s;
    this->normalmap = n;
}

ON_3dVector
cmesh_t::tnorm(const triangle_t &t)
{
    ON_3dPoint *p1 = this->pnts[t.v[0]];
    ON_3dPoint *p2 = this->pnts[t.v[1]];
    ON_3dPoint *p3 = this->pnts[t.v[2]];

    ON_3dVector e1 = *p2 - *p1;
    ON_3dVector e2 = *p3 - *p1;
    ON_3dVector tdir = ON_CrossProduct(e1, e2);
    tdir.Unitize();
    return tdir;
}

ON_3dPoint
cmesh_t::tcenter(const triangle_t &t)
{
    ON_3dPoint avgpnt(0,0,0);

    for (size_t i = 0; i < 3; i++) {
	ON_3dPoint *p3d = this->pnts[t.v[i]];
	avgpnt = avgpnt + *p3d;
    }

    ON_3dPoint cpnt = avgpnt/3.0;
    return cpnt;
}

ON_3dVector
cmesh_t::bnorm(const triangle_t &t)
{
    ON_3dPoint avgnorm(0,0,0);

    // Can't calculate this without some key Brep data
    if (!this->normalmap) return avgnorm;

    double norm_cnt = 0.0;

    for (size_t i = 0; i < 3; i++) {
	ON_3dPoint *p3d = this->pnts[t.v[i]];
	if (singularities->find(p3d) != singularities->end()) {
	    // singular vert norms are a product of multiple faces - not useful for this
	    continue;
	}
	ON_3dPoint onrm(*(*normalmap)[p3d]);
	if (this->m_bRev) {
	    onrm = onrm * -1;
	}
	avgnorm = avgnorm + onrm;
	norm_cnt = norm_cnt + 1.0;
    }

    ON_3dVector anrm = avgnorm/norm_cnt;
    anrm.Unitize();
    return anrm;
}

void cmesh_t::reset()
{
    this->tris.clear();
    this->v2edges.clear();
    this->v2tris.clear();
    this->edges2tris.clear();
    this->uedges2tris.clear();
    this->type = 0;
}

void cmesh_t::build_3d(std::set<p2t::Triangle *> *cdttri, std::map<p2t::Point *, ON_3dPoint *> *pointmap)
{
    if (!cdttri || !pointmap) return;

    this->reset();
    this->pnts_2d.clear();
    this->pnts.clear();
    this->p2ind.clear();
 
    std::set<p2t::Triangle*>::iterator s_it;

    // 3D mesh
    this->type = 0;

    // Populate points
    std::set<ON_3dPoint *> uniq_p3d;
    for (s_it = cdttri->begin(); s_it != cdttri->end(); s_it++) {
	p2t::Triangle *t = *s_it;
	for (size_t j = 0; j < 3; j++) {
	    uniq_p3d.insert((*pointmap)[t->GetPoint(j)]);
	}
    }
    this->sv.clear();
    std::set<ON_3dPoint *>::iterator u_it;
    for (u_it = uniq_p3d.begin(); u_it != uniq_p3d.end(); u_it++) {
	this->pnts.push_back(*u_it);
	this->p2ind[*u_it] = this->pnts.size() - 1;
	if (this->singularities->find(*u_it) != this->singularities->end()) {
	    this->sv.insert(this->p2ind[*u_it]);
	}
    }

    // From the triangles, populate the containers
    for (s_it = cdttri->begin(); s_it != cdttri->end(); s_it++) {
	p2t::Triangle *t = *s_it;
	ON_3dPoint *pt_A = (*pointmap)[t->GetPoint(0)];
	ON_3dPoint *pt_B = (*pointmap)[t->GetPoint(1)];
	ON_3dPoint *pt_C = (*pointmap)[t->GetPoint(2)];

	// Skip degenerate triangles
	if (pt_A == pt_B || pt_B == pt_C || pt_C == pt_A) {
	    continue;
	}

	long Aind = this->p2ind[pt_A];
	long Bind = this->p2ind[pt_B];
	long Cind = this->p2ind[pt_C];
	struct triangle_t nt(Aind, Bind, Cind);
	cmesh_t::tri_add(nt, 0);
    }

}

bool
cmesh_t::tri_problem_edges(triangle_t &t)
{
    if (!problem_edges.size()) return false;

    uedge_t ue1(t.v[0], t.v[1]);
    uedge_t ue2(t.v[1], t.v[2]);
    uedge_t ue3(t.v[2], t.v[0]);

    if (problem_edges.find(ue1) != problem_edges.end()) return true;
    if (problem_edges.find(ue2) != problem_edges.end()) return true;
    if (problem_edges.find(ue3) != problem_edges.end()) return true;
    return false;
}

size_t
cmesh_t::collect_neighbor_tris(triangle_t &seed, double deg, cmesh_t *submesh)
{
    double angle = deg * ON_PI/180.0;
    ON_3dVector sn = bnorm(seed);

    submesh->visited_triangles.insert(seed);

    std::queue<triangle_t> q1, q2;
    std::queue<triangle_t> *tq, *nq;
    tq = &q1;
    nq = &q2;

    {
	std::set<triangle_t> fvneigh;
	std::set<triangle_t>::iterator f_it;
	for (int i = 0; i < 3; i++) {
	    std::vector<triangle_t> fneigh = vertex_face_neighbors(seed.v[i]);
	    fvneigh.insert(fneigh.begin(), fneigh.end());
	}
	for (f_it = fvneigh.begin(); f_it != fvneigh.end(); f_it++) {
	    q1.push(*f_it);
	}
    }
    while (!tq->empty()) {
	triangle_t ct = tq->front();
	tq->pop();
	// Check normal
	ON_3dVector tn = bnorm(ct);
	double dprd = ON_DotProduct(sn, tn);
	double dang = (NEAR_EQUAL(dprd, 1.0, ON_ZERO_TOLERANCE)) ? 0 : acos(dprd);

	if (dang > angle) {
	    std::cerr << "Angle rejection (" << dang << "," << angle << ")\n";
	    continue;
	}

	// TODO - if triangle's non active vertex is inside the current
	// boundary loop, it needs to be stored as an interior point - no
	// triangle is added to the submesh, as the submesh's only purpose is
	// to provide the outer bounding polygon for p2t.  However, the
	// triangles associated with the interior point MUST be added to the
	// mesh, or problems will result
	//
	// In principle this might happen with a triangle flipped relative to
	// its brep normals...

	// TODO - if triangle has problem edges, it cannot be added as a
	// triangle - its non-active vertex needs to be stored as an interior
	// point, and the mesh must grow until that 2D point is inside the 2D
	// loop.  This will usually happen quickly, but isn't guaranteed to...
	//
	// One option might be to do a post-repair re-assessment, and if any
	// new problem edges have appeared re-seed with those triangles...
	//
	// Another might be an "all or nothing" policy for uedges with more than
	// two triangles...


	// TODO -reject triangle if tri_add indicates it would self-intersect
	// the boundary loop.  Remove it from visited if that's why we're
	// rejecting it, since subsequent additions might make it viable again.
	submesh->tri_add(ct, 1);


	// TODO grow this not by vertex or face neighbors, but by getting the
	// triangle sets from the current boundary_loop's uedge segments.
	// In essence, march the boundary loop out in steps rather than 
	// once face at a time.
	{
	    std::set<triangle_t> fvneigh;
	    std::set<triangle_t>::iterator f_it;
	    for (int i = 0; i < 3; i++) {
		std::vector<triangle_t> fneigh = vertex_face_neighbors(seed.v[i]);
		fvneigh.insert(fneigh.begin(), fneigh.end());
	    }
	    for (f_it = fvneigh.begin(); f_it != fvneigh.end(); f_it++) {
		if (submesh->visited_triangles.find(*f_it) == submesh->visited_triangles.end()) {
		    nq->push(*f_it);
		    submesh->visited_triangles.insert(*f_it);
		}
	    }
	}

	if (tq->empty()) {
	    std::queue<triangle_t> *tmpq = tq;
	    tq = nq;
	    nq = tmpq;
	}
    }

    return submesh->tris.size();
}

void
cmesh_t::remesh_tri(triangle_t &seed)
{
    std::set<long> interior_pnts;



    cmesh_t submesh;
    submesh.set_brep_data(this->m_bRev, this->edge_pnts, this->singularities, this->normalmap);
    submesh.type = 1; // 2D mesh

    // It's a 2D mesh, but go ahead and copy the 3D points in case anything
    // needs to refer to them
    submesh.pnts.assign(pnts.begin(), pnts.end());

    ON_3dPoint sp = this->tcenter(seed);
    ON_3dVector sn = this->bnorm(seed);
    ON_Plane tplane(sp, sn);
    ON_Xform to_plane;
    to_plane.PlanarProjection(tplane);
    for (size_t i = 0; i < pnts.size(); i++) {
	ON_3dPoint op3d = (*pnts[i]);
	op3d.Transform(to_plane);
	ON_2dPoint *n2d = new ON_2dPoint(op3d.x, op3d.y);
	submesh.pnts_2d.push_back(n2d);
    }

    // Grow submesh
    //
    // TODO - the "seeding" problem is a little more complex than just
    // picking a triangle.  What we actually need is a seed LOOP, which
    // we keep valid.  If the initial triangle can't give us that loop
    // (which it sometimes can't - say if it's flipped) we
    // need to use neighbor information to establish an initial valid
    // loop.  Probably pull all the tris sharing at least one vert
    // with the seed, eliminate all the edges sharing a vertex with
    // the seed, and try to build a loop out of the others. For any
    // bad faces in the neighbors, pull their neighbors and do the
    // same thing until we get a loop that doesn't have bad edges.
    // Brep edges are hard stops (obviously).  Any points not used
    // in the boundary need to be interior points for p2t
    if (!tri_problem_edges(seed)) {
	submesh.tri_add(seed, 0);
    } else {
	// Any seed triangle vertices not on the parent mesh boundary
	// will need to be interior in the remesh.
	for (int i = 0; i < 3; i++) {
	    if (sv.find(seed.v[i]) != sv.end()) continue;
	    ON_3dPoint *p = pnts[seed.v[i]];
	    if (edge_pnts->find(p) != edge_pnts->end()) continue;
	    interior_pnts.insert(seed.v[i]);
	}
	std::cout << "got " << interior_pnts.size() << " interior pnts\n";
    }
    double deg = 10;
    size_t ncnt = collect_neighbor_tris(seed, deg, &submesh);
    while (ncnt < 10 && deg < 45) {
	submesh.reset();
	if (!tri_problem_edges(seed)) {
	    submesh.tri_add(seed, 0);
	} else {
	    // Any seed triangle vertices not on the parent mesh boundary
	    // will need to be interior in the remesh.
	    for (int i = 0; i < 3; i++) {
		if (sv.find(seed.v[i]) != sv.end()) continue;
		ON_3dPoint *p = pnts[seed.v[i]];
		if (edge_pnts->find(p) != edge_pnts->end()) continue;
		interior_pnts.insert(seed.v[i]);
	    }
	}
	deg = deg + 5;
	ncnt = collect_neighbor_tris(seed, deg, &submesh);
    }

    // Re-triangulate submesh and replace the triangles in the current mesh


    // Clean up
    for (size_t i = 0; i < pnts_2d.size(); i++) {
	delete pnts_2d[i];
    }
    seed_tris.erase(seed);
}

void
cmesh_t::repair()
{
    std::vector<triangle_t> s_tris = this->singularity_triangles();
    std::vector<triangle_t> f_tris = this->interior_incorrect_normals(1);
    seed_tris.clear(); 
    seed_tris.insert(s_tris.begin(), s_tris.end());
    seed_tris.insert(f_tris.begin(), f_tris.end());

    while (seed_tris.size()) {
	triangle_t seed = *seed_tris.begin();

	remesh_tri(seed);
    }
}

void cmesh_t::plot_uedge(struct uedge_t &ue, FILE* plot_file)
{
    if (this->type == 0) {
	// 3D
	ON_3dPoint *p1 = this->pnts[ue.v[0]];
	ON_3dPoint *p2 = this->pnts[ue.v[1]];
	point_t bnp1, bnp2;
	VSET(bnp1, p1->x, p1->y, p1->z);
	VSET(bnp2, p2->x, p2->y, p2->z);
	pdv_3move(plot_file, bnp1);
	pdv_3cont(plot_file, bnp2);
    }
    if (this->type == 1) {
	// 2D
	ON_2dPoint *p1 = this->pnts_2d[ue.v[0]];
	ON_2dPoint *p2 = this->pnts_2d[ue.v[1]];
	point_t bnp1, bnp2;
	VSET(bnp1, p1->x, p1->y, 0);
	VSET(bnp2, p2->x, p2->y, 0);
	pdv_3move(plot_file, bnp1);
	pdv_3cont(plot_file, bnp2);
    }
}

void cmesh_t::boundary_edges_plot(const char *filename)
{
    FILE* plot_file = fopen(filename, "w");
    struct bu_color c = BU_COLOR_INIT_ZERO;
    bu_color_rand(&c, BU_COLOR_RANDOM_LIGHTENED);
    pl_color_buc(plot_file, &c);

    std::set<uedge_t> bedges = this->boundary_edges(1);
    std::set<uedge_t>::iterator b_it;
    for (b_it = bedges.begin(); b_it != bedges.end(); b_it++) {
	uedge_t ue = *b_it;
	plot_uedge(ue, plot_file);
    }

    if (this->problem_edges.size()) {
	pl_color(plot_file, 255, 0, 0);
	for (b_it = problem_edges.begin(); b_it != problem_edges.end(); b_it++) {
	    uedge_t ue = *b_it;
	    plot_uedge(ue, plot_file);
	}
    }

    fclose(plot_file);
}


void cmesh_t::boundary_loops_plot(int use_brep_data, const char *filename)
{
    FILE* plot_file = fopen(filename, "w");

    std::vector<std::vector<long>> loops = this->boundary_loops(use_brep_data);
    point_t bnp;
    for (size_t i = 0; i < loops.size(); i++) {
	struct bu_color c = BU_COLOR_INIT_ZERO;
	bu_color_rand(&c, BU_COLOR_RANDOM_LIGHTENED);
	pl_color_buc(plot_file, &c);

	if (this->type == 0) {
	    ON_3dPoint *p = this->pnts[loops[i][0]];
	    VSET(bnp, p->x, p->y, p->z);
	    pdv_3move(plot_file, bnp);
	}
	if (this->type == 1) {
	    ON_2dPoint *p = this->pnts_2d[loops[i][0]];
	    VSET(bnp, p->x, p->y, 0);
	    pdv_3move(plot_file, bnp);
	}
	for (size_t j = 1; j < loops[i].size(); j++) {
	    if (this->type == 0) {
		// 3D
		ON_3dPoint *p = this->pnts[loops[i][j]];
		VSET(bnp, p->x, p->y, p->z);
		pdv_3cont(plot_file, bnp);
	    }
	    if (this->type == 1) {
		// 2D
		ON_2dPoint *p = this->pnts_2d[loops[i][j]];
		VSET(bnp, p->x, p->y, 0);
		pdv_3cont(plot_file, bnp);
	    }
	}
    }

    fclose(plot_file);
}

void cmesh_t::plot_tri(const triangle_t &t, struct bu_color *buc, FILE *plot, int r, int g, int b)
{
    point_t p[3];
    point_t porig;
    point_t c = VINIT_ZERO;
    if (this->type == 0) {
	for (int i = 0; i < 3; i++) {
	    ON_3dPoint *p3d = this->pnts[t.v[i]];
	    VSET(p[i], p3d->x, p3d->y, p3d->z);
	    c[X] += p3d->x;
	    c[Y] += p3d->y;
	    c[Z] += p3d->z;
	}
	c[X] = c[X]/3.0;
	c[Y] = c[Y]/3.0;
	c[Z] = c[Z]/3.0;
    }

    if (this->type == 1) {
    	for (int i = 0; i < 3; i++) {
	    ON_2dPoint *p2d = this->pnts_2d[t.v[i]];
	    VSET(p[i], p2d->x, p2d->y, 0);
	    c[X] += p2d->x;
	    c[Y] += p2d->y;
	}
	c[X] = c[X]/3.0;
	c[Y] = c[Y]/3.0;
	c[Z] = 0;
    }

    for (size_t i = 0; i < 3; i++) {
	if (i == 0) {
	    VMOVE(porig, p[i]);
	    pdv_3move(plot, p[i]);
	}
	pdv_3cont(plot, p[i]);
    }
    pdv_3cont(plot, porig);

    /* fill in the "interior" using the rgb color*/
    pl_color(plot, r, g, b);
    for (size_t i = 0; i < 3; i++) {
	pdv_3move(plot, p[i]);
	pdv_3cont(plot, c);
    }


    /* Plot the triangle normal */
    pl_color(plot, 0, 255, 255);
    {
	ON_3dVector tn = this->tnorm(t);
	vect_t tnt;
	VSET(tnt, tn.x, tn.y, tn.z);
	point_t npnt;
	VADD2(npnt, tnt, c);
	pdv_3move(plot, c);
	pdv_3cont(plot, npnt);
    }

    /* Plot the brep normal */
    pl_color(plot, 0, 100, 0);
    {
	ON_3dVector tn = this->bnorm(t);
	tn = tn * 0.5;
	vect_t tnt;
	VSET(tnt, tn.x, tn.y, tn.z);
	point_t npnt;
	VADD2(npnt, tnt, c);
	pdv_3move(plot, c);
	pdv_3cont(plot, npnt);
    }

    /* restore previous color */
    pl_color_buc(plot, buc);
}

void cmesh_t::face_neighbors_plot(const triangle_t &f, const char *filename)
{
    FILE* plot_file = fopen(filename, "w");

    struct bu_color c = BU_COLOR_INIT_ZERO;
    bu_color_rand(&c, BU_COLOR_RANDOM_LIGHTENED);
    pl_color_buc(plot_file, &c);

    // Origin triangle has red interior
    std::vector<triangle_t> faces = this->face_neighbors(f);
    this->plot_tri(f, &c, plot_file, 255, 0, 0);

    // Neighbor triangles have blue interior
    for (size_t i = 0; i < faces.size(); i++) {
	triangle_t tri = faces[i];
	this->plot_tri(tri, &c, plot_file, 0, 0, 255);
    }

    fclose(plot_file);
}

void cmesh_t::vertex_face_neighbors_plot(long vind, const char *filename)
{
    FILE* plot_file = fopen(filename, "w");

    struct bu_color c = BU_COLOR_INIT_ZERO;
    bu_color_rand(&c, BU_COLOR_RANDOM_LIGHTENED);
    pl_color_buc(plot_file, &c);

    std::vector<triangle_t> faces = this->vertex_face_neighbors(vind);

    for (size_t i = 0; i < faces.size(); i++) {
	triangle_t tri = faces[i];
	this->plot_tri(tri, &c, plot_file, 0, 0, 255);
    }

    // Plot the vind point that is the source of the triangles
    pl_color(plot_file, 0, 255,0);
    if (this->type == 0) {
	ON_3dPoint *p = this->pnts[vind];
	vect_t pt;
	VSET(pt, p->x, p->y, p->z);
	pdv_3point(plot_file, pt);
    }
    if (this->type == 1) {
   	ON_2dPoint *p = this->pnts_2d[vind];
	pd_point(plot_file, p->x, p->y);
    }
    fclose(plot_file);
}

void cmesh_t::interior_incorrect_normals_plot(const char *filename)
{
    FILE* plot_file = fopen(filename, "w");

    struct bu_color c = BU_COLOR_INIT_ZERO;
    bu_color_rand(&c, BU_COLOR_RANDOM_LIGHTENED);
    pl_color_buc(plot_file, &c);

    std::vector<triangle_t> faces = this->interior_incorrect_normals(1);
    for (size_t i = 0; i < faces.size(); i++) {
	this->plot_tri(faces[i], &c, plot_file, 0, 255, 0);
    }
    fclose(plot_file);
}

void cmesh_t::tri_plot(triangle_t &tri, const char *filename)
{
    FILE* plot_file = fopen(filename, "w");

    struct bu_color c = BU_COLOR_INIT_ZERO;
    bu_color_rand(&c, BU_COLOR_RANDOM_LIGHTENED);
    pl_color_buc(plot_file, &c);

    this->plot_tri(tri, &c, plot_file, 255, 0, 0);

    fclose(plot_file);
}

void cmesh_t::tris_set_plot(std::set<triangle_t> &tset, const char *filename)
{
    FILE* plot_file = fopen(filename, "w");

    struct bu_color c = BU_COLOR_INIT_ZERO;
    bu_color_rand(&c, BU_COLOR_RANDOM_LIGHTENED);
    pl_color_buc(plot_file, &c);

    std::set<triangle_t>::iterator s_it;

    for (s_it = tset.begin(); s_it != tset.end(); s_it++) {
	triangle_t tri = (*s_it);
	this->plot_tri(tri, &c, plot_file, 255, 0, 0);
    }
    fclose(plot_file);
}

void cmesh_t::tris_plot(const char *filename)
{
    this->tris_set_plot(this->tris, filename);
}

}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8