#pragma once

#include <fstream>
#include <cstring>

#include <assert.h>

#include <boost/crc.hpp>

#include "db2_hardware_difference.h"
#include "db2_container.h"
#include "db2_structure_reflector.h"

/*

*/

template <typename T>
class db2Chunk : public db2DynArray<T>
{
public:
    ENDIAN_SENSITIVE; // int32_t length{0};  // defined in base
    char type[4]{'N', 'U', 'L', 'L'};
    ENDIAN_SENSITIVE; // T *data{nullptr};  // defined in base
    ENDIAN_SENSITIVE int32_t CRC{0};

private:
    bool isLittleEndian{hardwareDifference::isLittleEndian()};
    db2StructReflector *reflector{nullptr};

public:
    db2Chunk(const char *type = nullptr)
    {
        if (type)
        {
            ::memcpy(this->type, type, 4);
            this->reflector = db2StructReflector::getReflector(this->type);
        }
    }

    db2Chunk(std::ifstream &fs, const bool &isLittleEndian)
    {
        this->read(fs, isLittleEndian);
    }

public:
    /* type-irrelative */
    auto read(std::ifstream &fs, const bool &isLittleEndian) -> void
    {
        assert(this->length == 0);

        // endian
        this->isLittleEndian = isLittleEndian;

        // length
        fs.read((char *)&this->length, sizeof(this->length));
        if (this->isLittleEndian != hardwareDifference::isLittleEndian())
            hardwareDifference::reverseEndian((char *)&this->length, sizeof(this->length));

        // type
        fs.read(this->type, sizeof(this->type));
        this->reflector = db2StructReflector::getReflector(this->type);

        // data
        this->reserve_men(this->length, false);
        fs.read((char *)this->data, this->length);

        // CRC
        fs.read((char *)&(this->CRC), sizeof(this->CRC));
        if (this->isLittleEndian != hardwareDifference::isLittleEndian())
            hardwareDifference::reverseEndian((char *)&this->CRC, sizeof(this->CRC));

        // do check CRC here and before reverseEndian()
        assert(this->calculateCRC(this->data, this->length) == this->CRC);

        // handle endian of data
        if (this->isLittleEndian != hardwareDifference::isLittleEndian())
        {
            this->reverseEndian();
            this->isLittleEndian = !this->isLittleEndian;
        }
    }

    /* type-irrelative */
    auto write(std::ofstream &fs, bool asLittleEndian = hardwareDifference::isLittleEndian()) -> void
    {
        auto length = this->length;
        if (asLittleEndian != this->isLittleEndian)
            hardwareDifference::reverseEndian((char *)&length, sizeof(length));

        auto data = this->data;
        if (asLittleEndian != this->isLittleEndian)
        {
            data = (char *)::malloc(this->length);
            ::memcpy(data, this->data, this->length);
            this->reverseEndian((char *)data);
        }

        // calculate CRC
        auto CRC = this->calculateCRC(data, this->length);
        if (asLittleEndian != this->isLittleEndian)
            hardwareDifference::reverseEndian((char *)&CRC, sizeof(CRC));

        // length
        fs.write((char *)&length, sizeof(length));
        // type
        fs.write((char *)this->type, 4);
        // data
        fs.write((char *)data, this->length);
        // CRC
        fs.write((char *)&CRC, sizeof(CRC));

        if (data != this->data)
            ::free(data);
    }

    /* type-irrelative, since reflector is adopted */
    auto reverseEndian(char *data = nullptr) -> void
    {
        if (!data)
            data = this->data;

        if (this->reflector == nullptr)
            return;

        for (int i = 0; i < this->length / reflector->length; ++i)
            for (int j = 0; j < this->reflector->offsets.size(); ++j)
                hardwareDifference::reverseEndian(
                    data + reflector->length * i + this->reflector->offsets[j],
                    this->reflector->lengths[j]);
    }

    auto calculateCRC(const void *const data, uint32_t length) -> const uint32_t
    {
        // boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc;
        boost::crc_32_type crc;
        crc.process_bytes(data, length);
        return crc.checksum();
    }

    auto calculateCRC_1(const void *const data, uint32_t length) -> const uint32_t
    {
        uint32_t crc = 0xFFFFFFFF; // Initial value

        const uint8_t *bytes = static_cast<const uint8_t *>(data);
        for (uint32_t i = 0; i < length; ++i)
        {
            uint8_t c = bytes[i];

            crc ^= static_cast<uint32_t>(c) << 24; // XOR operation
            for (int j = 0; j < 8; ++j)
            {
                if (crc & 0x80000000)
                    crc = (crc << 1) ^ 0x04C11DB7; // Shift and XOR operation
                else
                    crc <<= 1;
            }
        }

        return ~crc; // Bitwise negation
    }

    auto calculateCRC_2(const void *const data, uint32_t length) -> const uint32_t
    {

        uint32_t crc = 0xFFFFFFFF; // 初始化CRC寄存器

        const uint8_t *bytes = static_cast<const uint8_t *>(data);

        for (size_t i = 0; i < length; ++i)
        {
            crc ^= bytes[i]; // 异或当前字节

            for (int j = 0; j < 8; ++j)
            {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1))); // 右移一位并进行异或
            }
        }

        return crc ^ 0xFFFFFFFF; // 取反作为最终结果
    }

    auto calculateCRC_3(const void *const data, uint32_t length) -> const uint32_t
    {
        uint32_t crc = 0xFFFFFFFF; // 初始值

        const uint8_t *bytes = static_cast<const uint8_t *>(data);

        for (uint32_t i = 0; i < length; ++i)
        {
            crc ^= bytes[i]; // 异或当前字节

            for (int j = 0; j < 8; ++j)
            {
                if (crc & 0x00000001)
                    crc = (crc >> 1) ^ 0xEDB88320; // 右移一位并进行异或
                else
                    crc >>= 1;
            }
        }

        return crc ^ 0xFFFFFFFF; // 取反作为最终结果
    }
};