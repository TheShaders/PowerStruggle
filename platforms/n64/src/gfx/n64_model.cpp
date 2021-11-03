#include <n64_model.h>
#include <model.h>

extern "C" {
#include <debug.h>
}

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
    // Skip empty material draws (could exist for the purpose of just switching material)
    if (num_groups == 0)
    {
        return 0;
    }
    size_t num_commands = 1; // end DL
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
    size_t num_commands = 0;
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

void Model::adjust_offsets()
{
    void *base_addr = this;
    debug_printf("Adjusting offsets of model at 0x%08X\n", base_addr);
    debug_printf("  Joints: %d Materials: %d\n", num_joints, num_materials);
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
        num_commands += materials[mat_idx]->gfx_length;
    }
    return num_commands;
}

Gfx *MaterialHeader::setup_gfx(Gfx *gfx_pos)
{
    int rand_val = guRandom() % 3;
    gDPPipeSync(gfx_pos++);
    switch (rand_val)
    {
        case 0:
            gDPSetEnvColor(gfx_pos++, 0xFF, 0xFF, 0xFF, 0xFF);
            break;
        case 1:
            gDPSetEnvColor(gfx_pos++, 0xFF, 0xFF, 0xFF, 0xFF);
            break;
        case 2:
            gDPSetEnvColor(gfx_pos++, 0xFF, 0x00, 0x00, 0xFF);
            break;
    }
    gDPSetCombineLERP(gfx_pos++, ENVIRONMENT, 0, SHADE, 0, 0, 0, 0, 1, ENVIRONMENT, 0, SHADE, 0, 0, 0, 0, 1);
    gSPEndDisplayList(gfx_pos++);
    return gfx_pos;
}

Gfx *MaterialDraw::setup_gfx(Gfx* gfx_pos, Vtx* verts) const
{
    // Skip empty material draws (could exist for the purpose of just switching material)
    if (num_groups == 0)
    {
        return gfx_pos;
    }
    // Iterate over every triangle group in this draw
    for (size_t group_idx = 0; group_idx < num_groups; group_idx++)
    {
        const auto& cur_group = groups[group_idx];
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
    // Terminate the draw's DL
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
            auto& cur_layer = joints[joint_idx].layers[layer];
            for (size_t draw_idx = 0; draw_idx < cur_layer.num_draws; draw_idx++)
            {
                cur_layer.draws[draw_idx].gfx = cur_gfx;
                cur_gfx = cur_layer.draws[draw_idx].setup_gfx(cur_gfx, verts);
            }
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
