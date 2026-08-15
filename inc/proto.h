#pragma once

#include <cstdint>
#include <vector>

namespace proto_project {

#pragma pack(push, 1)
struct PacketHeader {
    uint16_t server_hash;
    uint16_t secret_key;
    uint32_t total_data_size;
    uint16_t total_cnt_packets;
    uint16_t cur_packet_number;
    uint16_t cur_packet_size;
    uint8_t is_first;
    uint8_t is_last;
    uint8_t is_compressed;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Packet {
    PacketHeader header;
    uint8_t data_type;
    std::vector<char> data;
    static std::vector<char> doSerialize( const Packet &src ) {
        std::vector<char> buffer;
        
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.server_hash), reinterpret_cast<const char*>(&src.header.server_hash) + 2);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.secret_key), reinterpret_cast<const char*>(&src.header.secret_key) + 2);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.total_data_size), reinterpret_cast<const char*>(&src.header.total_data_size) + 4);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.total_cnt_packets), reinterpret_cast<const char*>(&src.header.total_cnt_packets) + 2);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.cur_packet_number), reinterpret_cast<const char*>(&src.header.cur_packet_number) + 2);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.cur_packet_size), reinterpret_cast<const char*>(&src.header.cur_packet_size) + 2);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.is_first), reinterpret_cast<const char*>(&src.header.is_first) + 1);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.is_last), reinterpret_cast<const char*>(&src.header.is_last) + 1);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.header.is_compressed), reinterpret_cast<const char*>(&src.header.is_compressed) + 1);
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&src.data_type), reinterpret_cast<const char*>(&src.data_type) + 1);
        buffer.insert(buffer.end(), src.data.begin(), src.data.end());

        return buffer;
    }
    static Packet doDeserialise( const std::vector<char> &src ) {
        Packet packet;

        size_t offset = 0;
        memcpy(&packet.header.server_hash, src.data() + offset, 2); 
        offset += 2;
        memcpy(&packet.header.secret_key, src.data() + offset, 2); 
        offset += 2;
        memcpy(&packet.header.total_data_size, src.data() + offset, 4); 
        offset += 4;
        memcpy(&packet.header.total_cnt_packets, src.data() + offset, 2); 
        offset += 2;
        memcpy(&packet.header.cur_packet_number, src.data() + offset, 2); 
        offset += 2;
        memcpy(&packet.header.cur_packet_size, src.data() + offset, 2); 
        offset += 2;
        memcpy(&packet.header.is_first, src.data() + offset, 1); 
        offset += 1;
        memcpy(&packet.header.is_last, src.data() + offset, 1); 
        offset += 1;
        memcpy(&packet.header.is_compressed, src.data() + offset, 1); 
        offset += 1;
        memcpy(&packet.data_type, src.data() + offset, 1); 
        offset += 1;
        packet.data.assign(src.begin() + offset, src.end());

        return packet;
    }
};
#pragma pack(pop)

}