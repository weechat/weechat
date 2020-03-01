/*
 * Copyright (C) 2020 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_TEST_UNIT_CORE_H
#define WEECHAT_TEST_UNIT_CORE_H

#define DATA_HASH "this is a test of hash function"
#define DATA_HASH_CRC32 "ef26fe3e"
#define DATA_HASH_MD5 "1197d121af621ac6a63cb8ef6b5dfa30"
#define DATA_HASH_SHA1 "799d818061175b400dc5aaeb14b8d32cdef32ff0"
#define DATA_HASH_SHA224 "637d21f3ba3f4e9fa9fb889dc990b31a658cb37b4aefb5144" \
    "70b016d"
#define DATA_HASH_SHA256 "b9a4c3393dfac4330736684510378851e581c68add8eca841" \
    "10c31a33e694676"
#define DATA_HASH_SHA384 "42853280be9b8409eed265f272bd580e2fbd448b7c7e236c7" \
    "f37dafec7906d51d982dc84ec70a4733eca49d86ac19455"
#define DATA_HASH_SHA512 "4469190d4e0d1fdc0afb6f408d9873c89b8ce89cc4db79fe0" \
    "58255c55ad6821fa5e9bb068f9e578c8ae7cc825d85ff99c439d59e439bc589d95620a" \
    "1e6b8ae6e"
#define DATA_HASH_SHA3_224 "26432a3a4ea998790be43386b1de417f88be43146a4af98" \
    "2a9627d10"
#define DATA_HASH_SHA3_256 "226e3830306711cf653c1661765c304b37038e7457c35dd" \
    "14fca0f6a8ba1d2e3"
#define DATA_HASH_SHA3_384 "77bc16f89c102efc783ddeccc71862fe919b66e1aaa88bd" \
    "2ba5f0bbe604fcb86c68f0e401d5d553597366cdd400595ba"
#define DATA_HASH_SHA3_512 "31dfb5fc8f30ac7007acddc4fce562d408706833d0d2af2" \
    "e5f61a179099592927ff7d100e278406c7f98d42575001e26e153b135c21f7ef5b00c8" \
    "cef93ca048d"
#define DATA_HASH_SALT "this is a salt of 32 bytes xxxxx"
#define DATA_HASH_PBKDF2_SHA1_1000 "85ce23c8873830df8f0a96aa82ae7d7635dad12" \
    "7"
#define DATA_HASH_PBKDF2_SHA1_100000 "f2c2a079007523df3a5a5c7b578073dff06ba" \
    "49f"
#define DATA_HASH_PBKDF2_SHA256_1000 "0eb0a795537a8c37a2d7d7e50a076e07c9a8e" \
    "e9aa281669381af99fad198997c"
#define DATA_HASH_PBKDF2_SHA256_100000 "b243ef60f9d9e7a6315f874053802cfb3b7" \
    "b5a3502d47cf7fd76b3ee5661fcff"
#define DATA_HASH_PBKDF2_SHA512_1000 "03d8e9e86f3bbe20b88a600a5aa15f8cfbee0" \
    "a402af301e1714c25467a32489c773c71eddf5aa39f42823ecc54c9e9b015517b5f3c0" \
    "19bae9463a2d8fe527882"
#define DATA_HASH_PBKDF2_SHA512_100000 "286cca5521b00a7398530fbf11a570401a8" \
    "0c901f1584cb493fcb75c49e5c30613c331ce318b34615a08be2fdcb9a3b4d3e9a5b62" \
    "e119db6941010533cdf73d3"

#endif /* WEECHAT_TEST_UNIT_CORE_H */
