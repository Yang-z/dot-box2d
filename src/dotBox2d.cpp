#include "dotBox2d.h"

#include "decoders/db2_decoder.h"
#include "decoders/db2_transcoder.h"

dotBox2d::dotBox2d(const char *file)
{
    if (!file)
        return;

    dotBox2d::load(file);
}

dotBox2d::~dotBox2d()
{
    if (this->p_b2w)
        delete this->p_b2w;

    if (this->p_db2ContactListener)
        delete this->p_db2ContactListener;

    if (this->p_db2OffstepListener)
        delete this->p_db2OffstepListener;
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
    this->head[3] = HardwareDifference::IsLittleEndian() ? 'd' : 'D';

    // read chunks
    while (fs.peek() != EOF)
    {
        auto &chunk = this->chunks.emplace();
        chunk.read(fs, isFileLittleEndian);
    };

    fs.close();
}

auto dotBox2d::save(const char *filePath, bool asLittleEndian) -> void
{
    std::ofstream fs{filePath, std::ios::binary | std::ios::out};
    if (!fs)
        return;

    // write head
    fs.write((char *)this->head, 3);
    fs.write(asLittleEndian ? "d" : "D", 1);
    fs.write((char *)this->head + 4, 4);

    // write chunks
    for (auto i = 0; i < this->chunks.size(); ++i)
        this->chunks[i].write(fs, asLittleEndian);

    fs.close();
}

auto dotBox2d::decode() -> void
{
    db2Decoder::Decode(*this);
    db2Transcoder::Transcode(*this);

    this->p_b2w->SetContactListener(this->p_db2ContactListener);
}

auto dotBox2d::encode() -> void
{
    db2Decoder::Encode(*this);
}

auto dotBox2d::step() -> void
{
    this->p_db2OffstepListener->PreStep();
    this->p_b2w->Step(this->dt, this->velocityIterations, this->positionIterations);
    this->p_db2OffstepListener->PostStep();
}