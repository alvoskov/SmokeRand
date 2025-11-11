#include <cstdint>
#include <cstring>
#include <cassert>
#include <array>
#include <cinttypes>
#include <cstdio>

template<size_t nrounds>
class ChaChaCore
{
protected:
    static constexpr size_t STATE_SIZE = 16;
    std::array<uint32_t, STATE_SIZE> x {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
        0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
        0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
        0x00000001, 0x09000000, 0x4a000000, 0x00000000
    }; ///< Working state
    std::array<uint32_t, STATE_SIZE> out; ///< Output state
    size_t pos = STATE_SIZE; ///< Current position inside the output buffer

    static inline uint32_t rotl32(uint32_t x, int r)
    {
        return (x << r) | (x >> ((-r) & 31));
    }

    inline void qround(size_t ai, size_t bi, size_t ci, size_t di)
    {
        out[ai] += out[bi]; out[di] ^= out[ai]; out[di] = rotl32(out[di], 16);
        out[ci] += out[di]; out[bi] ^= out[ci]; out[bi] = rotl32(out[bi], 12);
        out[ai] += out[bi]; out[di] ^= out[ai]; out[di] = rotl32(out[di], 8);
        out[ci] += out[di]; out[bi] ^= out[ci]; out[bi] = rotl32(out[bi], 7);
    }

    void generateBlock()
    {
        std::copy(x.begin(), x.end(), out.begin());
        for (size_t k = 0; k < nrounds / 2; k++) {
            // Vertical qrounds
            qround(0, 4, 8,12);
            qround(1, 5, 9,13);
            qround(2, 6,10,14);
            qround(3, 7,11,15);
            // Diagonal qrounds
            qround(0, 5,10,15);
            qround(1, 6,11,12);
            qround(2, 7, 8,13);
            qround(3, 4, 9,14);
        }
        for (size_t i = 0; i < STATE_SIZE; i++) {
            out[i] += x[i];
        }
    }
    
public:
    using result_type = uint32_t;
    
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT32_MAX; }
    
    result_type operator()()
    {
        if (pos >= STATE_SIZE) {
            if (++x[12] == 0) { x[13]++; }
            generateBlock();
            pos = 0;
        }
        return out[pos++];
    }

    void print() const
    {
        std::printf("ChaCha initial state\n");
        for (size_t i = 0; i < 4; i++) {
            for (size_t j = 0; j < 4; j++) {
                size_t ind = 4 * i + j;
                std::printf("%.8" PRIX32 " ", x[ind]);
            }
            std::printf("\n");
        }
    }
};

class ChaCha20 : public ChaChaCore<20>
{
    bool selfTest()
    {
        static const uint32_t x_init[] = { // Input values
            0x03020100,  0x07060504,  0x0b0a0908,  0x0f0e0d0c,
            0x13121110,  0x17161514,  0x1b1a1918,  0x1f1e1d1c,
            0x00000001,  0x09000000,  0x4a000000,  0x00000000
        };

        static const uint32_t out_final[] = { // Refernce values from RFC 7359
           0xe4e7f110,  0x15593bd1,  0x1fdd0f50,  0xc47120a3,
           0xc7f4d1c7,  0x0368c033,  0x9aaa2204,  0x4e6cd4c3,
           0x466482d2,  0x09aa9f07,  0x05d7c214,  0xa2028bd9,
           0xd19c12b5,  0xb94e16de,  0xe883d0cb,  0x4e3c50a2
        };
        for (size_t i = 0; i < 4; i++) {
            x[i + 12] = x_init[i + 8];
        }
        generateBlock();
        for (size_t i = 0; i < STATE_SIZE; i++) {
            if (out_final[i] != out[i]) {
                return false;
            }
        }
        return true;
    }

public:
    ChaCha20(const std::array<uint32_t, 8> &key, uint64_t nonce)
    {
        assert(selfTest() && "ChaCha20 implementation is broken");
        for (size_t i = 0; i < 8; i++) {
            x[i + 4] = key[i];
        }
        x[12] = 0;
        x[13] = 0;
        x[14] = (uint32_t) nonce;
        x[15] = (uint32_t) (nonce >> 32);
    }
};
