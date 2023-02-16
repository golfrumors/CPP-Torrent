#include <assert.h>
#include <stdint.h>

//AVX intrinsic functions header
#include <immintrin.h>

#include "include/Utils.h"

//fast memory copy util func, only available on AVX2 compliant cpus
void fastMemcpy(void* ptrDest, void* ptrSrc, size_t nBytes) {
        assert(nBytes % 32 == 0);
        assert((intptr_t(ptrDest) & 31) == 0);
        assert((intptr_t(ptrSrc) & 31) == 0);
        const __m256i *pSrc = reinterpret_cast<const __m256i*>(ptrSrc);
        __m256i *pDest = reinterpret_cast<__m256i*>(ptrDest);
        int64_t nVects = nBytes / sizeof(*pSrc);
        for (; nVects > 0; nVects--, pSrc++, pDest++) {
            const __m256i loaded = _mm256_stream_load_si256(pSrc);
            _mm256_stream_si256(pDest, loaded);
        }
        _mm_sfence();
}
