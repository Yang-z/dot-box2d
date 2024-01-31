#include "db2_structure.h"

#include <fstream>
#include <algorithm>

#include <assert.h>

#include <type_traits> // std::is_same

bool db2ChunkType::IsRegistered = db2ChunkType::RegisterType();

auto db2ChunkType::RegisterType() -> bool
{
    db2Reflector::Reflect<db2Chunk<dotB2Info>>(db2ChunkType::INFO);
    db2Reflector::Reflect<db2Chunk<dotB2Wrold>>(db2ChunkType::WRLD);
    db2Reflector::Reflect<db2Chunk<dotB2Joint>>(db2ChunkType::JInT);
    db2Reflector::Reflect<db2Chunk<dotB2Body>>(db2ChunkType::BODY);
    db2Reflector::Reflect<db2Chunk<dotB2Fixture>>(db2ChunkType::FXTR);
    db2Reflector::Reflect<db2Chunk<dotB2Shape>>(db2ChunkType::SHpE);

    // db2Reflector::Reflect<db2Chunk<int32_t>>(db2ChunkType::DIcT);
    // db2Reflector::Reflect<db2Chunk<float32_t>>(db2ChunkType::LIsT);
    // db2Reflector::Reflect<db2Chunk<char>>(db2ChunkType::CHAR);

    return true;
}

dotBox2d::dotBox2d(const char *file)
{
    if (!file)
        return;

    dotBox2d::load(file);
}

dotBox2d::~dotBox2d()
{
    for (auto i = 0; i < this->chunks.size(); ++i)
        delete this->chunks[i];
}

auto dotBox2d::load(const char *filePath) -> void
{
    std::ifstream fs{filePath, std::ios::binary};
    if (!fs)
        return;

    // read head
    fs.read((char *)&(this->head), sizeof(this->head));

    // confirm endia
    const bool isFileLittleEndian = (this->head[3] == 'd');

    // change to local endian
    this->head[3] = hardwareDifference::IsLittleEndian() ? 'd' : 'D';

    // read chunk
    while (fs.peek() != EOF)
    {
        this->chunks.push(new db2Chunk<char>{fs, isFileLittleEndian});
        // this->chunks[-1]->read(fs, isFileLittleEndian);
    };

    fs.close();
}

auto dotBox2d::save(const char *filePath, bool asLittleEndian) -> void
{
    std::ofstream fs{filePath, std::ios::binary | std::ios::out};
    if (!fs)
        return;

    this->head[3] = asLittleEndian ? 'd' : 'D';
    fs.write((char *)&(this->head), sizeof(this->head));
    this->head[3] = hardwareDifference::IsLittleEndian() ? 'd' : 'D';

    for (auto i = 0; i < this->chunks.size(); ++i)
        this->chunks[i]->write(fs, asLittleEndian);

    fs.close();
}