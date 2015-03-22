#include <openvdb/openvdb.h>
#include <iostream>
#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/VolumeToMesh.h>

using namespace std;

void build_test_voxel(float** Data, int& Num)
{
    // Initialize the OpenVDB library.  This must be called at least
    // once per program and may safely be called multiple times.
    openvdb::initialize();
    
    openvdb::FloatGrid::Ptr grid =
        openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(
            /*radius=*/50.0, /*center=*/openvdb::Vec3f(1.5, 2, 3),
            /*voxel size=*/4, /*width=*/4.0);

    vector<openvdb::Vec3s> pnts;
    vector<openvdb::Vec3I> tris;
    vector<openvdb::Vec4I> quads;
    openvdb::tools::volumeToMesh(*grid, pnts, tris, quads, 0, 1);

    for (int i=0; i<quads.size(); i++) {
        tris.push_back(openvdb::Vec3I(quads[i][0], quads[i][1], quads[i][2]));
        tris.push_back(openvdb::Vec3I(quads[i][0], quads[i][2], quads[i][3]));
    }
    
    Num = tris.size() * 9;
    *Data = new float[Num];
    int idx = 0;
    for (int i=0; i<tris.size(); i++)
    for (int j=0; j<3; j++)
    for (int k=0; k<3; k++)
        (*Data)[idx++] = pnts[tris[i][j]][k];
}