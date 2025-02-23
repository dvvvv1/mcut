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

#ifndef MCUT_KERNEL_H
#define MCUT_KERNEL_H
#include <mcut/internal/halfedge_mesh.h>

#include <map>
#include <vector>

namespace mcut {

//
// final execution states (i.e. did anything go wrong..?)
//
enum class status_t {
    SUCCESS = 0,
    // mesh is malformed
    // * vertices less than 3
    // * no faces
    // * non-manifold
    // * contains more than one connected component
    INVALID_SRC_MESH = -1, // TODO: these error flags should be generated in mcut.cpp not the kernel
    INVALID_CUT_MESH = -2,
    //EDGE_EDGE_INTERSECTION = -3, // Found an edge-edge intersection.
    //FACE_VERTEX_INTERSECTION = -4, // Found an face-vertex intersection.

    // there exists no edge in the input mesh which intersects cut-surface polygon
    INVALID_MESH_INTERSECTION = -3,

    // The bounding volume heirarchies of the input mesh and cut surface do not overlap
    //INVALID_BVH_INTERSECTION = -6,
    /*
        MCUT is formulated for inputs in general position. Here the notion of general position is defined with
        respect to the orientation predicate (as evaluated on the intersecting polygons). Thus, a set of points 
        is in general position if no three points are collinear and also no four points are coplanar.

        MCUT uses the "GENERAL_POSITION_VIOLATION" flag to inform of when to use perturbation (of the
        cut-mesh) so as to bring the input into general position. In such cases, the idea is to solve the cutting
        problem not on the given input, but on a nearby input. The nearby input is obtained by perturbing the given
        input. The perturbed input will then be in general position and, since it is near the original input,
        the result for the perturbed input will hopefully still be useful.  This is justified by the fact that
        the task of MCUT is not to decide whether the input is in general position but rather to make perturbation
        on the input (if) necessary within the available precision of the computing device.
    */
    GENERAL_POSITION_VIOLATION = -4,
    /*
      TODO: add documentation
    */
    DETECTED_FLOATING_POLYGON = -5
};

//
// Position of a cut surface patch with respect to the input mesh
//
enum class cut_surface_patch_location_t : unsigned char {
    INSIDE, // + : The patch is located inside the input mesh volume (i.e. it is used to seal holes)
    OUTSIDE, // - : The patch is located outside the input mesh volume (boolean union).
    UNDEFINED // ~ : The notion of INSIDE/OUTSIDE is not applicable because input mesh is non-watertight
};

//
// Position of a connected component (CC) relative to cut-surface
//
enum class connected_component_location_t : unsigned char {
    ABOVE, // + : The CC is on positive side of the cut-surface (normal direction)
    BELOW, // - :  The CC is on negative side of the cut-surface (normal direction)
    UNDEFINED // ~ : The notion of ABOVE/BELOW is not applicable because the CC has [partially] cut
};

//
// The winding order of the polygons of a cut surface patch
//
enum class cut_surface_patch_winding_order_t : unsigned char {
    DEFAULT, // + : The polygons of the patch have the [same] winding order as the cut-surface (e.g. CCW)
    REVERSE, // - : The polygons of the patch have the [opposite] winding order as the cut-surface (e.g. CW)
};

struct floating_polygon_info_t {
    // the input mesh (source-mesh or cut-mesh) containing the face on which the floating polygon was discovered.
    // This is a pointer to input_t::src_mesh or input_t::cut_mesh
    // const mesh_t* origin_mesh = nullptr;
    // largest component of the normal of the origin_face
    int projection_component = -1;
    // the positions of the vertices of the floating polygon (order implies connectivity i.e. two points next to each other share a vertex)
    std::vector<math::vec3> polygon_vertices;
};

//
// settings for how to execute the function "mcut::dispatch(...)"
//
struct input_t {
    const mesh_t* src_mesh = nullptr;
    const mesh_t* cut_mesh = nullptr;
    const std::vector<std::pair<fd_t, fd_t>>* intersecting_sm_cm_face_pairs = nullptr;
    bool verbose = true;
    //bool keep_partially_sealed_connected_components = false;
    bool require_looped_cutpaths = false; // ... i.e. bail on partial cuts (any!)
    bool populate_vertex_maps = false; // compute data relating vertices in cc to original input mesh
    bool populate_face_maps = false; // compute data relating face in cc to original input mesh
    bool enforce_general_position = false;
    // counts how many times we have perturbed the cut-mesh to enforce general-position
    int general_position_enforcement_count = 0;

    // NOTE TO SELF: if the user simply wants seams, then kernel should not have to proceed to stitching!!!
    bool keep_srcmesh_seam = false;
    bool keep_cutmesh_seam = false;
    //
    bool keep_unsealed_fragments = false;
    //
    bool keep_inside_patches = false;
    bool keep_outside_patches = false;
    //
    bool keep_fragments_below_cutmesh = false;
    bool keep_fragments_above_cutmesh = false;
    //
    bool keep_fragments_partially_cut = false;
    bool keep_fragments_sealed_inside = false;
    bool keep_fragments_sealed_outside = false;
    // bool include_fragment_sealed_partial = false; // See: variable above "keep_partially_sealed_connected_components"
    bool keep_fragments_sealed_inside_exhaustive = false;
    bool keep_fragments_sealed_outside_exhaustive = false;
    // NOTE TO SELF: if the user simply wants patches, then kernel should not have to proceed to stitching!!!
};

struct output_mesh_data_maps_t {
    std::map<
        vd_t, // vertex descriptor in connected component
        vd_t // vertex descriptor in input-mesh e.g. source mesh or cut mesh. ("null_vertex()" if vertex is an intersection point)
        >
        vertex_map;
    std::map<
        fd_t, // face descriptor in connected component
        fd_t // face descriptor in input-mesh e.g. source mesh or cut mesh. (new polygons resulting from clipping are mapped to the same input mesh face)
        >
        face_map;
};

struct output_mesh_info_t {
    mesh_t mesh;
    std::vector<vd_t> seam_vertices;
    output_mesh_data_maps_t data_maps;
};

//
// the output returned from the function "mcut::dispatch"
//
struct output_t {
    status_t status = status_t::SUCCESS;
    logger_t logger;
    // fragments
    std::map<connected_component_location_t, std::map<cut_surface_patch_location_t, std::vector<output_mesh_info_t>>> connected_components;
    std::map<connected_component_location_t, std::vector<output_mesh_info_t>> unsealed_cc; // connected components before hole-filling
    // patches
    std::map<cut_surface_patch_winding_order_t, std::vector<output_mesh_info_t>> inside_patches; // .. between neigbouring connected ccsponents (cs-sealing patches)
    std::map<cut_surface_patch_winding_order_t, std::vector<output_mesh_info_t>> outside_patches;
    // the input meshes which also include the edges that define the cut path
    // NOTE: not always defined (depending on the arising cutpath configurations)
    output_mesh_info_t seamed_src_mesh;
    output_mesh_info_t seamed_cut_mesh;

    // floating polygon handling
    std::map<
        // the face of the origin-mesh on which floating polygon(s) are discovered
        // NOTE: this is a descriptor into the polygon soup. Thus, we'll need to
        // subtract the number of source-mesh faces if this face belongs to the cut
        // mesh.
        fd_t,
        // info about floating polygons contained on ps-face
        std::vector<floating_polygon_info_t>>
        detected_floating_polygons;
};

//
// returns string equivalent value (e.g. for printing)
//
std::string to_string(const connected_component_location_t&);
std::string to_string(const cut_surface_patch_location_t&);
std::string to_string(const status_t&);
std::string to_string(const cut_surface_patch_winding_order_t&);

void dispatch(output_t& out, const input_t& in);

} // namespace mcut

#endif // #ifndef MCUT_KERNEL_H
