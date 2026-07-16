// kawr — mesh export to binary glTF (.glb). Apache-2.0.
#include "MeshExport.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void putU32(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back(uint8_t(v & 0xff));
    b.push_back(uint8_t((v >> 8) & 0xff));
    b.push_back(uint8_t((v >> 16) & 0xff));
    b.push_back(uint8_t((v >> 24) & 0xff));
}

void putF32(std::vector<uint8_t>& b, float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    putU32(b, u);
}

struct Panel {
    std::vector<float> pos;     // x,y,z per vertex
    std::vector<uint32_t> idx;  // triangle indices, local to this panel
    float lo[3] = {0, 0, 0};
    float hi[3] = {0, 0, 0};
    size_t posByteOffset = 0;
    size_t idxByteOffset = 0;
    std::string name;
};

// Build front + back panels for one shape, triangulating each quad. The back
// panel reverses winding so its faces point outward.
void addPanels(const Shape& sh, size_t s, std::vector<Panel>& out) {
    if (sh.meshVerts.empty() || sh.quadIdx.empty()) return;

    for (int side = 0; side < 2; ++side) {
        Panel p;
        p.name = "shape" + std::to_string(s) + (side == 0 ? "_front" : "_back");
        for (const Vec3& v0 : sh.meshVerts) {
            Vec3 v = (side == 0) ? v0 : v0 + sh.offset;
            p.pos.push_back(float(v.x));
            p.pos.push_back(float(v.y));
            p.pos.push_back(float(v.z));
        }
        for (size_t q = 0; q + 4 <= sh.quadIdx.size(); q += 4) {
            uint32_t a = sh.quadIdx[q], b = sh.quadIdx[q + 1];
            uint32_t c = sh.quadIdx[q + 2], d = sh.quadIdx[q + 3];
            if (side == 0)
                p.idx.insert(p.idx.end(), {a, b, c, a, c, d});
            else
                p.idx.insert(p.idx.end(), {a, c, b, a, d, c});
        }
        // Bounding box (required by glTF for POSITION accessors).
        for (int k = 0; k < 3; ++k) { p.lo[k] = p.pos[k]; p.hi[k] = p.pos[k]; }
        for (size_t i = 0; i < p.pos.size(); i += 3)
            for (int k = 0; k < 3; ++k) {
                float c = p.pos[i + k];
                if (c < p.lo[k]) p.lo[k] = c;
                if (c > p.hi[k]) p.hi[k] = c;
            }
        out.push_back(std::move(p));
    }
}

}  // namespace

ExportStats exportGlb(const std::vector<Shape>& shapes, const std::string& path) {
    ExportStats st;

    std::vector<Panel> panels;
    for (size_t s = 0; s < shapes.size(); ++s) addPanels(shapes[s], s, panels);

    // Pack the binary buffer: all positions, then all indices.
    std::vector<uint8_t> binPos, binIdx;
    for (Panel& p : panels) {
        p.posByteOffset = binPos.size();
        for (float f : p.pos) putF32(binPos, f);
        p.idxByteOffset = binIdx.size();
        for (uint32_t i : p.idx) putU32(binIdx, i);
        st.verts += int(p.pos.size() / 3);
        st.quads += int(p.idx.size() / 6);  // 2 triangles per source quad
    }
    st.panels = int(panels.size());
    size_t posBytes = binPos.size();
    size_t idxBytes = binIdx.size();

    std::vector<uint8_t> bin;
    bin.reserve(posBytes + idxBytes);
    bin.insert(bin.end(), binPos.begin(), binPos.end());
    bin.insert(bin.end(), binIdx.begin(), binIdx.end());

    // glTF JSON.
    std::ostringstream js;
    js << "{\"asset\":{\"version\":\"2.0\",\"generator\":\"kawr\"}";
    if (panels.empty()) {
        js << ",\"scene\":0,\"scenes\":[{\"nodes\":[]}]}";
    } else {
        js << ",\"scene\":0,\"scenes\":[{\"nodes\":[";
        for (size_t i = 0; i < panels.size(); ++i) { if (i) js << ','; js << i; }
        js << "]}],\"nodes\":[";
        for (size_t i = 0; i < panels.size(); ++i) {
            if (i) js << ',';
            js << "{\"mesh\":" << i << ",\"name\":\"" << panels[i].name << "\"}";
        }
        js << "],\"meshes\":[";
        for (size_t i = 0; i < panels.size(); ++i) {
            if (i) js << ',';
            js << "{\"name\":\"" << panels[i].name
               << "\",\"primitives\":[{\"attributes\":{\"POSITION\":" << (2 * i)
               << "},\"indices\":" << (2 * i + 1) << ",\"mode\":4}]}";
        }
        js << "],\"accessors\":[";
        for (size_t i = 0; i < panels.size(); ++i) {
            const Panel& p = panels[i];
            if (i) js << ',';
            js << "{\"bufferView\":0,\"byteOffset\":" << p.posByteOffset
               << ",\"componentType\":5126,\"count\":" << (p.pos.size() / 3)
               << ",\"type\":\"VEC3\",\"min\":[" << p.lo[0] << ',' << p.lo[1] << ','
               << p.lo[2] << "],\"max\":[" << p.hi[0] << ',' << p.hi[1] << ','
               << p.hi[2] << "]},";
            js << "{\"bufferView\":1,\"byteOffset\":" << p.idxByteOffset
               << ",\"componentType\":5125,\"count\":" << p.idx.size()
               << ",\"type\":\"SCALAR\"}";
        }
        js << "],\"bufferViews\":["
           << "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":" << posBytes
           << ",\"target\":34962},"
           << "{\"buffer\":0,\"byteOffset\":" << posBytes << ",\"byteLength\":"
           << idxBytes << ",\"target\":34963}],"
           << "\"buffers\":[{\"byteLength\":" << bin.size() << "}]}";
    }
    std::string json = js.str();

    while (json.size() % 4 != 0) json.push_back(' ');   // JSON chunk pad: spaces
    while (bin.size() % 4 != 0) bin.push_back(0);        // BIN chunk pad: zeros

    std::ofstream out(path, std::ios::binary);
    if (!out) return st;

    bool hasBin = !bin.empty();
    uint32_t jsonLen = uint32_t(json.size());
    uint32_t binLen = uint32_t(bin.size());
    uint32_t total = 12 + 8 + jsonLen + (hasBin ? (8 + binLen) : 0);

    std::vector<uint8_t> head;
    putU32(head, 0x46546C67);  // magic "glTF"
    putU32(head, 2);           // version
    putU32(head, total);
    putU32(head, jsonLen);
    putU32(head, 0x4E4F534A);  // "JSON"
    out.write(reinterpret_cast<const char*>(head.data()), std::streamsize(head.size()));
    out.write(json.data(), std::streamsize(json.size()));
    if (hasBin) {
        std::vector<uint8_t> ch;
        putU32(ch, binLen);
        putU32(ch, 0x004E4942);  // "BIN\0"
        out.write(reinterpret_cast<const char*>(ch.data()), std::streamsize(ch.size()));
        out.write(reinterpret_cast<const char*>(bin.data()), std::streamsize(bin.size()));
    }

    st.ok = bool(out);
    return st;
}
