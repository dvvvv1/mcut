/**
 * Copyright (c) 2020-2021 CutDigital Ltd.
 * All rights reserved.
 * 
 * NOTE: This file is licensed under GPL-3.0-or-later (default). 
 * A commercial license can be purchased from CutDigital Ltd. 
 *  
 * License details:
 * 
 * (A)  GNU General Public License ("GPL"); a copy of which you should have 
 *      recieved with this file.
 * 	    - see also: <http://www.gnu.org/licenses/>
 * (B)  Commercial license.
 *      - email: contact@cut-digital.com
 * 
 * The commercial license options is for users that wish to use MCUT in 
 * their products for comercial purposes but do not wish to release their 
 * software products under the GPL license. 
 * 
 * Author(s)     : Floyd M. Chitalu
 */

#include "mcut/internal/halfedge_mesh.h"

#include <algorithm>
#include <cstdio>

namespace mcut {

mesh_t::mesh_t() { }
mesh_t::~mesh_t() { }

// static member functions
// -----------------------

vertex_descriptor_t mesh_t::null_vertex()
{
    return vertex_descriptor_t();
}

halfedge_descriptor_t mesh_t::null_halfedge()
{
    return halfedge_descriptor_t();
}

edge_descriptor_t mesh_t::null_edge()
{
    return edge_descriptor_t();
}

face_descriptor_t mesh_t::null_face()
{
    return face_descriptor_t();
}

// regular member functions
// ------------------------

int mesh_t::number_of_vertices() const
{
    return number_of_internal_vertices() - number_of_vertices_removed();
}

int mesh_t::number_of_edges() const
{
    return number_of_internal_edges() - number_of_edges_removed();
}

int mesh_t::number_of_halfedges() const
{
    return number_of_internal_halfedges() - number_of_halfedges_removed();
}

int mesh_t::number_of_faces() const
{
    return number_of_internal_faces() - number_of_faces_removed();
}

vertex_descriptor_t mesh_t::source(const halfedge_descriptor_t& h) const
{
    MCUT_ASSERT(h != null_halfedge());
    const halfedge_data_t& hd = m_halfedges.at(h);
    MCUT_ASSERT(hd.o != null_halfedge());
    const halfedge_data_t& ohd = m_halfedges.at(hd.o); // opposite
    return ohd.t;
}

vertex_descriptor_t mesh_t::target(const halfedge_descriptor_t& h) const
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    const halfedge_data_t& hd = m_halfedges.at(h);
    return hd.t;
}

halfedge_descriptor_t mesh_t::opposite(const halfedge_descriptor_t& h) const
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    const halfedge_data_t& hd = m_halfedges.at(h);
    return hd.o;
}

halfedge_descriptor_t mesh_t::prev(const halfedge_descriptor_t& h) const
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    const halfedge_data_t& hd = m_halfedges.at(h);
    return hd.p;
}

halfedge_descriptor_t mesh_t::next(const halfedge_descriptor_t& h) const
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    const halfedge_data_t& hd = m_halfedges.at(h);
    return hd.n;
}

void mesh_t::set_next(const halfedge_descriptor_t& h, const halfedge_descriptor_t& nxt)
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(nxt != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    halfedge_data_t& hd = m_halfedges.at(h);
    hd.n = nxt;
    set_previous(nxt, h);
}

void mesh_t::set_previous(const halfedge_descriptor_t& h, const halfedge_descriptor_t& prev)
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(prev != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    halfedge_data_t& hd = m_halfedges.at(h);
    hd.p = prev;
}

edge_descriptor_t mesh_t::edge(const halfedge_descriptor_t& h) const
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    const halfedge_data_t& hd = m_halfedges.at(h);
    return hd.e;
}

face_descriptor_t mesh_t::face(const halfedge_descriptor_t& h) const
{
    MCUT_ASSERT(h != null_halfedge());
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    const halfedge_data_t& hd = m_halfedges.at(h);
    return hd.f;
}

vertex_descriptor_t mesh_t::vertex(const edge_descriptor_t e, const int v) const
{
    MCUT_ASSERT(e != null_edge());
    MCUT_ASSERT(v == 0 || v == 1);
    MCUT_ASSERT(m_edges.count(e) == 1);
    const edge_data_t& ed = m_edges.at(e);
    const halfedge_descriptor_t h = ed.h;
    MCUT_ASSERT(m_halfedges.count(h) == 1);
    const halfedge_data_t& hd = m_halfedges.at(h);
    vertex_descriptor_t v_out = hd.t; // assuming v ==0

    if (v == 1) {
        const halfedge_descriptor_t opp = hd.o;
        MCUT_ASSERT(m_halfedges.count(opp) == 1);
        const halfedge_data_t& ohd = m_halfedges.at(opp);
        v_out = ohd.t;
    }

    return v_out;
}

bool mesh_t::is_border(const halfedge_descriptor_t h)
{
    MCUT_ASSERT(h != null_halfedge());
    return face(h) == null_face();
}

bool mesh_t::is_border(const edge_descriptor_t e)
{
    MCUT_ASSERT(e != null_edge());
    halfedge_descriptor_t h0 = halfedge(e, 0);
    MCUT_ASSERT(h0 != null_halfedge());
    halfedge_descriptor_t h1 = halfedge(e, 1);
    MCUT_ASSERT(h1 != null_halfedge());

    return is_border(h0) || is_border(h1);
}

halfedge_descriptor_t mesh_t::halfedge(const edge_descriptor_t e, const int i) const
{
    MCUT_ASSERT(i == 0 || i == 1);
    MCUT_ASSERT(e != null_edge());
    MCUT_ASSERT(m_edges.count(e) == 1);
    const edge_data_t& ed = m_edges.at(e);
    halfedge_descriptor_t h = ed.h; // assuming i ==0

    MCUT_ASSERT(h != null_halfedge());

    if (i == 1) {
        MCUT_ASSERT(m_halfedges.count(h) == 1);

        const halfedge_data_t& hd = m_halfedges.at(h);
        h = hd.o;

        MCUT_ASSERT(h != null_halfedge());
    }

    return h;
}

halfedge_descriptor_t mesh_t::halfedge(const vertex_descriptor_t s, const vertex_descriptor_t t, bool strict_check) const
{
    MCUT_ASSERT(m_vertices.count(s) == 1);
    const vertex_data_t& svd = m_vertices.at(s);
    const std::vector<halfedge_descriptor_t>& s_halfedges = svd.m_halfedges;
    MCUT_ASSERT(m_vertices.count(t) == 1);
    const vertex_data_t& tvd = m_vertices.at(t);
    const std::vector<halfedge_descriptor_t>& t_halfedges = tvd.m_halfedges;
    std::vector<edge_descriptor_t> t_edges;
    t_edges.reserve(t_halfedges.size());

    for (std::vector<halfedge_descriptor_t>::const_iterator i = t_halfedges.cbegin(); i != t_halfedges.cend(); i++) {
        edge_descriptor_t e = edge(*i);
        MCUT_ASSERT(e != null_edge());
        t_edges.push_back(e);
    }

    halfedge_descriptor_t result = null_halfedge();
    for (std::vector<halfedge_descriptor_t>::const_iterator i = s_halfedges.cbegin(); i != s_halfedges.cend(); ++i) {
        edge_descriptor_t s_edge = edge(*i);
        if (std::find(t_edges.cbegin(), t_edges.cend(), s_edge) != t_edges.cend()) // belong to same edge?
        {
            result = *i; // assume source(*i) and target(*i) match "s" and "t"

            // check if we need to return the opposite halfedge
            if ((source(*i) == s && target(*i) == t) == false) {
                MCUT_ASSERT(source(*i) == t);
                MCUT_ASSERT(target(*i) == s);

                halfedge_descriptor_t h = opposite(*i);

                if (face(h) != null_face() || strict_check) { // "strict_check" ensures that we return the halfedge matching the input vertices
                    result = h;
                    break;
                }
            }
        }
    }
    return result;
}

edge_descriptor_t mesh_t::edge(const vertex_descriptor_t s, const vertex_descriptor_t t, bool strict_check) const
{
    halfedge_descriptor_t h = halfedge(s, t, strict_check);
    return (h == mesh_t::null_halfedge() ? mesh_t::null_edge() : edge(h));
}

vertex_descriptor_t mesh_t::add_vertex(const math::vec3& point)
{
    return add_vertex(point.x(), point.y(), point.z());
}

#if defined(MCUT_WITH_ARBITRARY_PRECISION_NUMBERS)
vertex_descriptor_t mesh_t::add_vertex(const math::fast_vec3& point)
{
    return add_vertex(point.x(), point.y(), point.z());
}
#endif // #if defined(MCUT_WITH_ARBITRARY_PRECISION_NUMBERS)

vertex_descriptor_t mesh_t::add_vertex(const math::real_number_t& x, const math::real_number_t& y, const math::real_number_t& z)
{
    vertex_descriptor_t vd = mesh_t::null_vertex();
    vertex_data_t* data_ptr = nullptr;
    bool reusing_removed_descr = (!m_vertices_removed.empty());

    if (reusing_removed_descr) // can we re-use a slot?
    {
        std::vector<vertex_descriptor_t>::const_iterator it = m_vertices_removed.cbegin(); // take the oldest unused slot (NOTE: important for user data mapping)
        vd = *it;
        m_vertices_removed.erase(it);
        MCUT_ASSERT(m_vertices.find(vd) != m_vertices.cend());
        data_ptr = &m_vertices.at(vd);
    } else {
        vd = static_cast<vertex_descriptor_t>(number_of_vertices());
        std::pair<typename std::map<vertex_descriptor_t, vertex_data_t>::iterator, bool> ret = m_vertices.insert(std::make_pair(vd, vertex_data_t()));
        MCUT_ASSERT(ret.second == true);
        data_ptr = &ret.first->second;
    }

    MCUT_ASSERT(vd != mesh_t::null_vertex());

    data_ptr->p = math::vec3(x, y, z);

    return vd;
}

#if !defined(MCUT_WITH_ARBITRARY_PRECISION_NUMBERS)
vertex_descriptor_t mesh_t::add_vertex(const char* x, const char* y, const char* z)
{
    // ... convert convert the numerical value embedded in each character-string to a double
    math::real_number_t x_ = static_cast<math::real_number_t>(std::atof(x));
    math::real_number_t y_ = static_cast<math::real_number_t>(std::atof(y));
    math::real_number_t z_ = static_cast<math::real_number_t>(std::atof(z));

    return add_vertex(x_, y_, z_); // register the vertex
}
#endif // #if !defined(MCUT_WITH_ARBITRARY_PRECISION_NUMBERS)

halfedge_descriptor_t mesh_t::add_edge(const vertex_descriptor_t v0, const vertex_descriptor_t v1)
{
    MCUT_ASSERT(v0 != null_vertex());
    MCUT_ASSERT(v1 != null_vertex());

    // primary halfedge(0) of edge
    halfedge_descriptor_t h0_idx(static_cast<face_descriptor_t::index_type>(number_of_halfedges())); // primary halfedge of new edge to be created
    bool reusing_removed_h0_descr = (!m_halfedges_removed.empty());

    if (reusing_removed_h0_descr) // can we re-use a slot?
    {
        std::vector<halfedge_descriptor_t>::const_iterator hIter = m_halfedges_removed.cbegin(); // take the oldest unused slot (NOTE: important for user data mapping)
        h0_idx = *hIter;
        m_halfedges_removed.erase(hIter);
        MCUT_ASSERT(m_halfedges.find(h0_idx) != m_halfedges.cend());
    }

    halfedge_data_t* halfedge0_data_ptr = nullptr;
    if (reusing_removed_h0_descr) {
        halfedge0_data_ptr = &m_halfedges.at(h0_idx);
    } else {
        // create new halfedge --> h0
        std::pair<typename std::map<halfedge_descriptor_t, halfedge_data_t>::iterator, bool> h0_ret = m_halfedges.insert(std::make_pair(h0_idx, halfedge_data_t()));
        MCUT_ASSERT(h0_ret.second == true);
        halfedge0_data_ptr = &h0_ret.first->second;
    }

    // second halfedge(1) of edge
    halfedge_descriptor_t h1_idx(static_cast<face_descriptor_t::index_type>(number_of_halfedges())); // second halfedge of new edge to be created (opposite of h0_idx)
    bool reusing_removed_h1_descr = (!m_halfedges_removed.empty());

    if (reusing_removed_h1_descr) // can we re-use a slot?
    {
        std::vector<halfedge_descriptor_t>::const_iterator hIter = m_halfedges_removed.cbegin() + (m_halfedges_removed.size() - 1); // take the most recently removed
        h1_idx = *hIter;
        m_halfedges_removed.erase(hIter);
        MCUT_ASSERT(m_halfedges.find(h1_idx) != m_halfedges.cend());
    }

    halfedge_data_t* halfedge1_data_ptr = nullptr;
    if (reusing_removed_h1_descr) {
        halfedge1_data_ptr = &m_halfedges.at(h1_idx);
    } else {
        // create new halfedge --> h1
        std::pair<typename std::map<halfedge_descriptor_t, halfedge_data_t>::iterator, bool> h1_ret = m_halfedges.insert(std::make_pair(h1_idx, halfedge_data_t()));
        MCUT_ASSERT(h1_ret.second == true);
        halfedge1_data_ptr = &h1_ret.first->second;
    }

    // edge

    edge_descriptor_t e_idx(static_cast<face_descriptor_t::index_type>(number_of_edges())); // index of new edge
    bool reusing_removed_edge_descr = (!m_edges_removed.empty());

    if (reusing_removed_edge_descr) // can we re-use a slot?
    {
        std::vector<edge_descriptor_t>::const_iterator eIter = m_edges_removed.cbegin(); // take the oldest unused slot (NOTE: important for user data mapping)
        e_idx = *eIter;
        m_edges_removed.erase(eIter);
        MCUT_ASSERT(m_edges.find(e_idx) != m_edges.cend());
    }

    edge_data_t* edge_data_ptr = nullptr;
    if (reusing_removed_edge_descr) {
        edge_data_ptr = &m_edges.at(e_idx);
    } else {
        std::pair<typename std::map<edge_descriptor_t, edge_data_t>::iterator, bool> eret = m_edges.insert(std::make_pair(e_idx, edge_data_t())); // create a new edge
        MCUT_ASSERT(eret.second == true);
        edge_data_ptr = &eret.first->second;
    }

    // update incidence information

    //edge_data_t& edge_data = eret.first->second;
    edge_data_ptr->h = h0_idx; // even/primary halfedge
    //eret.first->h = h0_idx; // even/primary halfedge

    //halfedge_data_t& halfedge0_data = h0_ret.first->second;
    halfedge0_data_ptr->t = v1; // target vertex of h0
    halfedge0_data_ptr->o = h1_idx; // ... because opp has idx differing by 1
    halfedge0_data_ptr->e = e_idx;

    //halfedge_data_t& halfedge1_data = h1_ret.first->second;
    halfedge1_data_ptr->t = v0; // target vertex of h1
    halfedge1_data_ptr->o = h0_idx; // ... because opp has idx differing by 1
    halfedge1_data_ptr->e = e_idx;

    //MCUT_ASSERT(h1_ret.first->first == halfedge0_data_ptr->o); // h1 comes just afterward (its index)

    // update vertex incidence

    // v0
    MCUT_ASSERT(m_vertices.count(v0) == 1);
    vertex_data_t& v0_data = m_vertices.at(v0);
    //MCUT_ASSERT();
    if (std::find(v0_data.m_halfedges.cbegin(), v0_data.m_halfedges.cend(), h1_idx) == v0_data.m_halfedges.cend()) {
        v0_data.m_halfedges.push_back(h1_idx); // halfedge whose target is v0
    }
    // v1
    MCUT_ASSERT(m_vertices.count(v1) == 1);
    vertex_data_t& v1_data = m_vertices.at(v1);
    //MCUT_ASSERT();
    if (std::find(v1_data.m_halfedges.cbegin(), v1_data.m_halfedges.cend(), h0_idx) == v1_data.m_halfedges.cend()) {
        v1_data.m_halfedges.push_back(h0_idx); // halfedge whose target is v1
    }

    return static_cast<halfedge_descriptor_t>(h0_idx); // return halfedge whose target is v1
}

face_descriptor_t mesh_t::add_face(const std::vector<vertex_descriptor_t>& vi)
{
    const int face_vertex_count = static_cast<int>(vi.size());
    MCUT_ASSERT(face_vertex_count >= 3);

    const int face_count = number_of_faces();
    face_data_t new_face_data;
    face_descriptor_t new_face_idx(static_cast<face_descriptor_t::index_type>(face_count));
    bool reusing_removed_face_descr = (!m_faces_removed.empty());

    if (reusing_removed_face_descr) // can we re-use a slot?
    {
        std::vector<face_descriptor_t>::const_iterator fIter = m_faces_removed.cbegin(); // take the oldest unused slot (NOTE: important for user data mapping)
        new_face_idx = *fIter;
        m_faces_removed.erase(fIter); // slot is going to be used again

        MCUT_ASSERT(m_faces.find(new_face_idx) != m_faces.cend());
    }

    face_data_t* face_data_ptr = reusing_removed_face_descr ? &m_faces.at(new_face_idx) : &new_face_data;
    face_data_ptr->m_halfedges.clear();

    for (int i = 0; i < face_vertex_count; ++i) {
        const vertex_descriptor_t v0 = vi.at(i); // i.e. src

        MCUT_ASSERT(v0 != null_vertex());

        const vertex_descriptor_t v1 = vi.at((i + 1) % face_vertex_count); // i.e. tgt

        MCUT_ASSERT(v1 != null_vertex());

        // check if edge exists between v0 and v1 (using halfedges incident to either v0 or v1)
        // TODO: use the halfedge(..., true) function

        vertex_data_t& v0_data = m_vertices.at(v0);
        vertex_data_t& v1_data = m_vertices.at(v1);

        bool connecting_edge_exists = false;
        halfedge_descriptor_t v0_h = null_halfedge();
        halfedge_descriptor_t v1_h = null_halfedge();

        for (int v0_h_iter = 0; v0_h_iter < static_cast<int>(v0_data.m_halfedges.size()); ++v0_h_iter) {
            v0_h = v0_data.m_halfedges.at(v0_h_iter);
            const edge_descriptor_t v0_e = edge(v0_h);

            for (int v1_h_iter = 0; v1_h_iter < static_cast<int>(v1_data.m_halfedges.size()); ++v1_h_iter) {

                v1_h = v1_data.m_halfedges.at(v1_h_iter);
                const edge_descriptor_t v1_e = edge(v1_h);
                const bool same_edge = (v0_e == v1_e);

                if (same_edge) {
                    connecting_edge_exists = true;
                    break;
                }
            }

            if (connecting_edge_exists) {
                break;
            }
        }

        // we use v1 in the following since v1 is the target (vertices are associated with halfedges which point to them)

        halfedge_data_t* v1_hd_ptr = nullptr; // refer to halfedge whose tgt is v1

        if (connecting_edge_exists) // edge connecting v0 and v1
        {
            MCUT_ASSERT(m_halfedges.count(v1_h) == 1);
            v1_hd_ptr = &m_halfedges.at(v1_h);

            face_data_ptr->m_halfedges.push_back(v1_h);

        } else { // there exists no edge between v0 and v1, so we create it
            const halfedge_descriptor_t h = add_edge(v0, v1);
            MCUT_ASSERT(m_halfedges.count(h) == 1);
            v1_hd_ptr = &m_halfedges.at(h);

            face_data_ptr->m_halfedges.push_back(h);
        }

        MCUT_ASSERT(v1_hd_ptr->f == null_face());

        v1_hd_ptr->f = new_face_idx; // associate halfedge with face
    }

    if (!reusing_removed_face_descr) {
        MCUT_ASSERT(m_faces.count(new_face_idx) == 0);
        m_faces.insert(std::make_pair(new_face_idx, *face_data_ptr));
    }

    // update halfedges (next halfedge)
    //const std::vector<halfedge_descriptor_t>& halfedges_around_new_face = get_halfedges_around_face(new_face_idx);
    const int num_halfedges = static_cast<int>(face_data_ptr->m_halfedges.size());

    for (int i = 0; i < num_halfedges; ++i) {
        const halfedge_descriptor_t h = face_data_ptr->m_halfedges.at(i);
        const halfedge_descriptor_t nh = face_data_ptr->m_halfedges.at((i + 1) % num_halfedges);
        set_next(h, nh);
    }

    return new_face_idx;
}

const math::vec3& mesh_t::vertex(const vertex_descriptor_t& vd) const
{
    MCUT_ASSERT(vd != null_vertex());
    MCUT_ASSERT(m_vertices.count(vd) == 1);
    const vertex_data_t& vdata = m_vertices.at(vd);
    return vdata.p;
}

std::vector<vertex_descriptor_t> mesh_t::get_vertices_around_face(const face_descriptor_t f) const
{
    MCUT_ASSERT(f != null_face());
    std::vector<vertex_descriptor_t> vertex_descriptors;

    const std::vector<halfedge_descriptor_t>& halfedges_on_face = get_halfedges_around_face(f);

    for (int i = 0; i < (int)halfedges_on_face.size(); ++i) {
        const halfedge_descriptor_t h = halfedges_on_face.at(i);
        MCUT_ASSERT(m_halfedges.count(h) == 1);
        const halfedge_data_t& hd = m_halfedges.at(h);
        vertex_descriptors.push_back(hd.t);
    }
    return vertex_descriptors;
}

const std::vector<halfedge_descriptor_t>& mesh_t::get_halfedges_around_face(const face_descriptor_t f) const
{
    MCUT_ASSERT(f != null_face());
    MCUT_ASSERT(m_faces.count(f) == 1);
    return m_faces.at(f).m_halfedges;
}

const std::vector<face_descriptor_t> mesh_t::get_faces_around_face(const face_descriptor_t f) const
{
    MCUT_ASSERT(f != null_face());

    std::vector<face_descriptor_t> faces_around_face;
    const std::vector<halfedge_descriptor_t>& halfedges_on_face = get_halfedges_around_face(f);

    for (int i = 0; i < (int)halfedges_on_face.size(); ++i) {

        const halfedge_descriptor_t h = halfedges_on_face.at(i);
        MCUT_ASSERT(m_halfedges.count(h) == 1);
        const halfedge_data_t& hd = m_halfedges.at(h);

        if (hd.o != null_halfedge()) {
            MCUT_ASSERT(m_halfedges.count(hd.o) == 1);
            const halfedge_data_t& ohd = m_halfedges.at(hd.o);

            if (ohd.f != null_face()) {
                faces_around_face.push_back(ohd.f);
            }
        }
    }
    return faces_around_face;
}

const std::vector<halfedge_descriptor_t>& mesh_t::get_halfedges_around_vertex(const vertex_descriptor_t v) const
{
    MCUT_ASSERT(v != mesh_t::null_vertex());
    MCUT_ASSERT(m_vertices.count(v) == 1);
    const vertex_data_t& vd = m_vertices.at(v);
    const std::vector<halfedge_descriptor_t>& incoming_halfedges = vd.m_halfedges;
    return incoming_halfedges;
}

mesh_t::vertex_iterator_t mesh_t::vertices_begin() const
{
    vertex_map_t::const_iterator it = m_vertices.cbegin();
    while (it != m_vertices.cend() && is_removed(it->first)) {
        ++it; // shift the pointer to the first valid mesh element
    }
    return vertex_iterator_t(it, this);
}

mesh_t::vertex_iterator_t mesh_t::vertices_end() const
{
    return vertex_iterator_t(m_vertices.cend(), this);
}

mesh_t::edge_iterator_t mesh_t::edges_begin() const
{
    edge_map_t::const_iterator it = m_edges.cbegin();
    while (it != m_edges.cend() && is_removed(it->first)) {
        ++it; // shift the pointer to the first valid mesh element
    }
    return edge_iterator_t(it, this);
}

mesh_t::edge_iterator_t mesh_t::edges_end() const
{
    return edge_iterator_t(m_edges.cend(), this);
}

mesh_t::halfedge_iterator_t mesh_t::halfedges_begin() const
{
    halfedge_map_t::const_iterator it = m_halfedges.cbegin();
    while (it != m_halfedges.cend() && is_removed(it->first)) {
        ++it; // shift the pointer to the first valid mesh element
    }
    return halfedge_iterator_t(it, this);
}

mesh_t::halfedge_iterator_t mesh_t::halfedges_end() const
{
    return halfedge_iterator_t(m_halfedges.cend(), this);
}

mesh_t::face_iterator_t mesh_t::faces_begin() const
{
    face_map_t::const_iterator it = m_faces.cbegin();
    while (it != m_faces.cend() && is_removed(it->first)) {
        ++it; // shift the pointer to the first valid mesh element
    }
    return face_iterator_t(it, this);
}

mesh_t::face_iterator_t mesh_t::faces_end() const
{
    return face_iterator_t(m_faces.cend(), this);
}

void write_off(const char* fpath, const mcut::mesh_t& mesh)
{

    std::ofstream outfile(fpath);

    if (!outfile.is_open()) {
        printf("error: could not open file: %s\n", fpath);
        std::exit(1);
    }

    //
    // file header
    //
    outfile << "OFF\n";

    //
    // #vertices, #faces, #edges
    //
    outfile << mesh.number_of_vertices() << " " << mesh.number_of_faces() << " " << 0 /*mesh.number_of_edges()*/ << "\n";

    //
    // vertices
    //

    for (mcut::mesh_t::vertex_iterator_t iter = mesh.vertices_begin(); iter != mesh.vertices_end(); ++iter) {
        //const vertex_data_t& vdata = iter.second;
        const math::vec3& point = mesh.vertex(*iter);
        outfile << (double)point.x() << " " << (double)point.y() << " " << (double)point.z() << "\n";
    }

    //
    // edges
    //

#if 0
    for (typename mesh_t::edge_iterator_t iter = mesh.edges_begin(); iter != mesh.edges_end(); ++iter) {
        const mesh_t::edge_descriptor_t ed = iter.first;
        const mesh_t::vertex_descriptor_t& v0 = vertex(ed, 0);
        const mesh_t::vertex_descriptor_t& v1 = vertex(ed, 1);
        // TODO
    }
#endif

    //
    // faces
    //
    for (mcut::mesh_t::face_iterator_t iter = mesh.faces_begin(); iter != mesh.faces_end(); ++iter) {
        //const typename mesh_t::face_descriptor_t& fd = iter.first;
        const std::vector<vertex_descriptor_t> vertices_around_face = mesh.get_vertices_around_face(*iter);

        MCUT_ASSERT(!vertices_around_face.empty());

        outfile << vertices_around_face.size() << " ";

        for (std::vector<vertex_descriptor_t>::const_iterator i = vertices_around_face.cbegin(); i != vertices_around_face.cend(); ++i) {
            outfile << (*i) << " ";
        }
        outfile << " \n";
    }

    outfile.close();
}

void read_off(mcut::mesh_t& mesh, const char* fpath)
{
    auto next_line = [&](std::ifstream& f, std::string& s) -> bool {
        while (getline(f, s)) {
            if (s.length() > 1 && s[0] != '#') {
                return true;
            }
        }
        return false;
    };

    std::ifstream infile(fpath);

    if (!infile.is_open()) {
        printf("error: could not open file: %s\n", fpath);
        std::exit(1);
    }

    //
    // file header
    //
    std::string header;
    if (!next_line(infile, header)) {
        printf("error: .off file header not found\n");
        std::exit(1);
    }

    if (header != "OFF") {
        printf("error: unrecognised .off file header\n");
        std::exit(1);
    }

    //
    // #vertices, #faces, #edges
    //
    std::string info;
    if (!next_line(infile, info)) {
        printf("error: .off element count not found\n");
        std::exit(1);
    }

    std::istringstream info_stream;
    info_stream.str(info);

    int nvertices;
    int nfaces;
    int nedges;
    info_stream >> nvertices >> nfaces >> nedges;

    //
    // vertices
    //
    std::map<int, vd_t> vmap;
    for (int i = 0; i < nvertices; ++i) {
        if (!next_line(infile, info)) {
            printf("error: .off vertex not found\n");
            std::exit(1);
        }
        std::istringstream vtx_line_stream(info);

        double x;
        double y;
        double z;
        vtx_line_stream >> x >> y >> z;
        vmap[i] = mesh.add_vertex(x, y, z);
    }

    //
    // edges
    //
    for (int i = 0; i < nedges; ++i) {
        // TODO
    }

    //
    // faces
    //
    for (auto i = 0; i < nfaces; ++i) {
        if (!next_line(infile, info)) {
            printf("error: .off file face not found\n");
            std::exit(1);
        }
        std::istringstream face_line_stream(info);
        int n; // number of vertices in face
        int index;
        face_line_stream >> n;

        if (n < 3) {
            printf("error: invalid polygon vertex count in file (%d)\n", n);
            std::exit(1);
        }

        typename std::vector<vd_t> face;
        face.resize(n);
        for (int j = 0; j < n; ++j) {
            info_stream >> index;
            face[j] = vmap[index];
        }

        mesh.add_face(face);
    }

    infile.close();
}

} // namespace mcut
