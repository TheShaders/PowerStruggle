#include <n64_model.h>

void TriangleGroup::adjust_offsets(void *base_addr)
{
    triangles = ::add_offset(triangles, base_addr);
}

void MaterialDraw::adjust_offsets(void *base_addr)
{
    groups = ::add_offset(groups, base_addr);
    for (size_t i = 0; i < num_groups; i++)
    {
        groups[i].adjust_offsets(base_addr);
    }
}

size_t MaterialDraw::gfx_length() const
{
    size_t num_commands = 1; // material load DL call
    for (size_t group_idx = 0; group_idx < num_groups; group_idx++)
    {
        num_commands += 
            1 + // vertex load
            (groups[group_idx].num_tris + 1) / 2; // tri commands, tris divided by 2 rounded up for the last 1tri
    }
    return num_commands;
}

void JointMeshLayer::adjust_offsets(void *base_addr)
{
    draws = ::add_offset(draws, base_addr);
    for (size_t draw_idx = 0; draw_idx < num_draws; draw_idx++)
    {
        draws[draw_idx].adjust_offsets(base_addr);
    }
}

size_t JointMeshLayer::gfx_length() const
{
    // Skip 0 draw layers
    if (num_draws == 0) return 0;
    size_t num_commands = 1; // end DL command
    for (size_t draw_idx = 0; draw_idx < num_draws; draw_idx++)
    {
        num_commands += draws[draw_idx].gfx_length();
    }
    return num_commands;
}

void Joint::adjust_offsets(void *base_addr)
{
    for (auto& layer : layers)
    {
        layer.adjust_offsets(base_addr);
    }
}

size_t Joint::gfx_length() const
{
    size_t num_commands = 0;
    for (const auto& layer : layers)
    {
        num_commands += layer.gfx_length();
    }
    return num_commands;
}

void Model::adjust_offsets(void *base_addr)
{
    joints    = ::add_offset(joints, base_addr);
    materials = ::add_offset(materials, base_addr);
    verts     = ::add_offset(verts, base_addr);
    for (size_t joint_idx = 0; joint_idx < num_joints; joint_idx++)
    {
        joints[joint_idx].adjust_offsets(base_addr);
    }
    for (size_t mat_idx = 0; mat_idx < num_materials; mat_idx++)
    {
        materials[mat_idx] = ::add_offset(materials[mat_idx], base_addr);
    }
}

size_t Model::gfx_length() const
{
    size_t num_commands = 0;
    for (size_t joint_idx = 0; joint_idx < num_joints; joint_idx++)
    {
        num_commands += joints[joint_idx].gfx_length();
    }
    for (size_t mat_idx = 0; mat_idx < num_materials; mat_idx++)
    {
        materials[mat_idx]->gfx_length = 3; // TODO temporary
        num_commands += materials[mat_idx]->gfx_length;
    }
    return num_commands;
}

Gfx *MaterialHeader::setup_gfx(Gfx *gfx_pos)
{
    gDPSetEnvColor(gfx_pos++, 0xFF, 0x00, 0x00, 0xFF);
    gDPSetCombineLERP(gfx_pos++, 0, 0, 0, ENVIRONMENT, 0, 0, 0, 1, 0, 0, 0, ENVIRONMENT, 0, 0, 0, 1);
    gSPEndDisplayList(gfx_pos++);
    return gfx_pos;
}

Gfx *JointMeshLayer::setup_gfx(Gfx* gfx_pos, Vtx* verts, MaterialHeader **materials) const
{
    // No need to waste space in the DL for an empty layer
    if (num_draws == 0)
        return gfx_pos;
    // Iterate over every material draw
    for (size_t draw_idx = 0; draw_idx < num_draws; draw_idx++)
    {
        const auto& cur_draw = draws[draw_idx];
        // Add the call to the material's DL
        gSPDisplayList(gfx_pos++, materials[cur_draw.material_index]->gfx);
        // Iterate over every triangle group in this draw
        for (size_t group_idx = 0; group_idx < cur_draw.num_groups; group_idx++)
        {
            const auto& cur_group = cur_draw.groups[group_idx];
            // Add the triangle group's vertex load
            gSPVertex(gfx_pos++, &verts[cur_group.load.start], cur_group.load.count, cur_group.load.buffer_offset);
            // Add the triangle group's triangle commands
            for (size_t tri2_index = 0; tri2_index < cur_group.num_tris / 2; tri2_index++)
            {
                const auto& tri1 = cur_group.triangles[tri2_index * 2 + 0];
                const auto& tri2 = cur_group.triangles[tri2_index * 2 + 1];
                gSP2Triangles(gfx_pos++,
                    tri1[0], tri1[1], tri1[2], 0x00,
                    tri2[0], tri2[1], tri2[2], 0x00);
            }
            if (cur_group.num_tris & 1) // If odd number of tris, add the last 1tri command
            {
                const auto& tri = cur_group.triangles[cur_group.num_tris - 1];
                gSP1Triangle(gfx_pos++, tri[0], tri[1], tri[2], 0x00);
            }
        }
    }
    // Terminate the joint layer's DL
    gSPEndDisplayList(gfx_pos++);
    return gfx_pos;
}

void Model::setup_gfx()
{
    // Allocate buffer to hold all joints gfx commands
    size_t num_gfx = gfx_length();
    // Use new instead of make_unique to avoid unnecessary initialization
    gfx = std::unique_ptr<Gfx[]>(new Gfx[num_gfx]);
    Gfx *cur_gfx = &gfx[0];
    // Set up the DL for every material
    for (size_t material_idx = 0; material_idx < num_materials; material_idx++)
    {
        materials[material_idx]->gfx = cur_gfx;
        cur_gfx = materials[material_idx]->setup_gfx(cur_gfx);
    }
    // Set up the DLs for every joint
    for (size_t joint_idx = 0; joint_idx < num_joints; joint_idx++)
    {
        for (size_t layer = 0; layer < gfx::draw_layers; layer++)
        {
            joints[joint_idx].layers[layer].gfx = cur_gfx;
            cur_gfx = joints[joint_idx].layers[layer].setup_gfx(cur_gfx, verts, materials);
        }
    }
    // infinite loop to check if my math was wrong
    if (cur_gfx - &gfx[0] != static_cast<ptrdiff_t>(num_gfx))
    {
        while (1);
    }
}


// Test data

template<class T, size_t N>
constexpr size_t array_size(T (&)[N]) { return N; }

constexpr Vtx make_vert_normal(int16_t x, int16_t y, int16_t z, int16_t s, int16_t t, int8_t nx, int8_t ny, int8_t nz)
{
    Vtx ret {};
    ret.n.ob[0] = x;
    ret.n.ob[1] = y;
    ret.n.ob[2] = z;
    ret.n.tc[0] = s;
    ret.n.tc[1] = t;
    ret.n.n[0] = nx;
    ret.n.n[1] = ny;
    ret.n.n[2] = nz;
    return ret;
}

Vtx cube_verts[] {
    make_vert_normal( 50, 50,  50,  0, 0,  0x00, 0x7F, 0x00),
    make_vert_normal( 50, 50, -50,  0, 0,  0x00, 0x7F, 0x00),
    make_vert_normal(-50, 50,  50,  0, 0,  0x00, 0x7F, 0x00),
    make_vert_normal(-50, 50, -50,  0, 0,  0x00, 0x7F, 0x00),
};

TriangleIndices cube_joint0_layer0_draw0_group0[] {
    { 0, 1, 2 },
    { 3, 2, 1 },
};

TriangleGroup cube_joint0_layer0_draw0_groups[] {
    {
        { // load
            0, // start
            4, // count
            0, // buffer_offset
        },
        array_size(cube_joint0_layer0_draw0_group0), // num_tris
        cube_joint0_layer0_draw0_group0 // triangles
    }
};

MaterialDraw cube_joint0_layer0_draws[] {
    {
        array_size(cube_joint0_layer0_draw0_groups), // num_groups
        0,
        cube_joint0_layer0_draw0_groups,
    }
};

Joint cube_joints[] {
    {
        0.0f, // posX
        0.0f, // posY
        0.0f, // posZ
        0xFF, // parent
        0, // reserved
        0, // reserved2
        {
            {},
            {
                array_size(cube_joint0_layer0_draws), // num_draws
                cube_joint0_layer0_draws, // draws
                nullptr // gfx
            },
            {},
            {},
            {},
            {}
        }
    }
};

MaterialHeader cube_mat0 {
    MaterialFlags::none,
    2,
    nullptr
};

MaterialHeader *cube_materials[] {
    &cube_mat0
};

Model cube_model {
    array_size(cube_joints), // num_joints
    array_size(cube_materials), // num_materials
    cube_joints, // joints
    cube_materials, // materials
    cube_verts, // verts
    nullptr, // gfx
};

Model *get_cube_model()
{
    cube_model.setup_gfx();
    return &cube_model;
}
