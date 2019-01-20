/* tools/mkbootimg/mkbootimg.c
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

#include "mincrypt/sha.h"
#include "mincrypt/sha256.h"
#include "bootimg.h"

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = sz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}

int usage(void)
{
    fprintf(stderr,"usage: mkbootimg\n"
            "       --kernel <filename>\n"
            "       [ --ramdisk <filename> ]\n"
            "       [ --second <2ndbootloader-filename> ]\n"
            "       [ --recovery_dtbo <recoverydtbo-filename> ]\n"
            "       [ --cmdline <kernel-commandline> ]\n"
            "       [ --board <boardname> ]\n"
            "       [ --base <address> ]\n"
            "       [ --pagesize <pagesize> ]\n"
            "       [ --dt <dtb-filename> ]\n"
            "       [ --kernel_offset <base offset> ]\n"
            "       [ --ramdisk_offset <base offset> ]\n"
            "       [ --second_offset <base offset> ]\n"
            "       [ --tags_offset <base offset> ]\n"
            "       [ --os_version <A.B.C version> ]\n"
            "       [ --os_patch_level <YYYY-MM-DD date> ]\n"
            "       [ --header_version <version number> ]\n"
            "       [ --hash <sha1(default)|sha256> ]\n"
            "       [ --id ]\n"
            "       -o|--output <filename>\n"
            );
    return 1;
}

static unsigned char padding[131072] = { 0, };

static void print_id(const uint8_t *id, size_t id_len)
{
    printf("0x");
    unsigned i = 0;
    for(i = 0; i < id_len; i++) {
        printf("%02x", id[i]);
    }
    printf("\n");
}

int write_padding(int fd, unsigned pagesize, unsigned itemsize)
{
    unsigned pagemask = pagesize - 1;
    ssize_t count;

    if((itemsize & pagemask) == 0) {
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    if(write(fd, padding, count) != count) {
        return -1;
    } else {
        return 0;
    }
}

int parse_os_version(char *ver)
{
    char *token;
    int verArray[3] = {0};
    int a,b,c = 0;
    int i = 0;

    token = strtok(ver, ".");
    while(token != NULL) {
        sscanf(token, "%d", &verArray[i]);
        token = strtok(NULL, ".");
        i++;
    }
    a = verArray[0];
    b = verArray[1];
    c = verArray[2];
    if((a < 128) && (b < 128) && (c < 128))
        return (a << 14) | (b << 7) | c;
    return 0;
}

int parse_os_patch_level(char *lvl)
{
    char *token;
    int lvlArray[2] = {0};
    int y,m = 0;
    int i = 0;

    token = strtok(lvl, "-");
    while(token != NULL) {
        sscanf(token, "%d", &lvlArray[i]);
        token = strtok(NULL, "-");
        i++;
    }
    y = lvlArray[0] - 2000;
    m = lvlArray[1];
    if((y >= 0) && (y < 128) && (m > 0) && (m <= 12))
        return (y << 4) | m;
    return 0;
}

enum hash_alg {
    HASH_UNKNOWN = -1,
    HASH_SHA1 = 0,
    HASH_SHA256,
};

struct hash_name {
    const char *name;
    enum hash_alg alg;
};

const struct hash_name hash_names[] = {
    { "sha1", HASH_SHA1 },
    { "sha256", HASH_SHA256 },
    { NULL, /* Sentinel */ },
};

enum hash_alg parse_hash_alg(char *name)
{
    const struct hash_name *ptr = hash_names;

    while(ptr->name) {
        if(!strcmp(ptr->name, name))
            return ptr->alg;
        ptr++;
    }

    return HASH_UNKNOWN;
}

void generate_id_sha1(boot_img_hdr_v1 *hdr, void *kernel_data, void *ramdisk_data,
                      void *second_data, void *dt_data, void *recovery_dtbo_data)
{
    SHA_CTX ctx;
    const uint8_t *sha;

    SHA_init(&ctx);
    SHA_update(&ctx, kernel_data, hdr->kernel_size);
    SHA_update(&ctx, &hdr->kernel_size, sizeof(hdr->kernel_size));
    SHA_update(&ctx, ramdisk_data, hdr->ramdisk_size);
    SHA_update(&ctx, &hdr->ramdisk_size, sizeof(hdr->ramdisk_size));
    SHA_update(&ctx, second_data, hdr->second_size);
    SHA_update(&ctx, &hdr->second_size, sizeof(hdr->second_size));
    if(dt_data) {
        SHA_update(&ctx, dt_data, hdr->dt_size);
        SHA_update(&ctx, &hdr->dt_size, sizeof(hdr->dt_size));
    } else if(hdr->header_version > 0) {
        SHA_update(&ctx, recovery_dtbo_data, hdr->recovery_dtbo_size);
        SHA_update(&ctx, &hdr->recovery_dtbo_size, sizeof(hdr->recovery_dtbo_size));
    }
    sha = SHA_final(&ctx);
    memcpy(hdr->id, sha, SHA_DIGEST_SIZE > sizeof(hdr->id) ? sizeof(hdr->id) : SHA_DIGEST_SIZE);
}

void generate_id_sha256(boot_img_hdr_v1 *hdr, void *kernel_data, void *ramdisk_data,
                        void *second_data, void *dt_data, void *recovery_dtbo_data)
{
    SHA256_CTX ctx;
    const uint8_t *sha;

    SHA256_init(&ctx);
    SHA256_update(&ctx, kernel_data, hdr->kernel_size);
    SHA256_update(&ctx, &hdr->kernel_size, sizeof(hdr->kernel_size));
    SHA256_update(&ctx, ramdisk_data, hdr->ramdisk_size);
    SHA256_update(&ctx, &hdr->ramdisk_size, sizeof(hdr->ramdisk_size));
    SHA256_update(&ctx, second_data, hdr->second_size);
    SHA256_update(&ctx, &hdr->second_size, sizeof(hdr->second_size));
    if(dt_data) {
        SHA256_update(&ctx, dt_data, hdr->dt_size);
        SHA256_update(&ctx, &hdr->dt_size, sizeof(hdr->dt_size));
    } else if(hdr->header_version > 0) {
        SHA256_update(&ctx, recovery_dtbo_data, hdr->recovery_dtbo_size);
        SHA256_update(&ctx, &hdr->recovery_dtbo_size, sizeof(hdr->recovery_dtbo_size));
    }
    sha = SHA256_final(&ctx);
    memcpy(hdr->id, sha, SHA256_DIGEST_SIZE > sizeof(hdr->id) ? sizeof(hdr->id) : SHA256_DIGEST_SIZE);
}

void generate_id(enum hash_alg alg, boot_img_hdr_v1 *hdr, void *kernel_data,
                 void *ramdisk_data, void *second_data, void *dt_data, void *recovery_dtbo_data)
{
    switch(alg) {
        case HASH_SHA1:
            generate_id_sha1(hdr, kernel_data, ramdisk_data, second_data, dt_data, recovery_dtbo_data);
            break;
        case HASH_SHA256:
            generate_id_sha256(hdr, kernel_data, ramdisk_data, second_data, dt_data, recovery_dtbo_data);
            break;
        case HASH_UNKNOWN:
        default:
            fprintf(stderr, "Unknown hash type.\n");
    }
}

int main(int argc, char **argv)
{
    boot_img_hdr_v1 hdr;

    char *kernel_fn = NULL;
    void *kernel_data = NULL;
    char *ramdisk_fn = NULL;
    void *ramdisk_data = NULL;
    char *second_fn = NULL;
    void *second_data = NULL;
    char *recovery_dtbo_fn = NULL;
    void *recovery_dtbo_data = NULL;
    char *cmdline = "";
    char *bootimg = NULL;
    char *board = "";
    int os_version = 0;
    int os_patch_level = 0;
    int header_version = 0;
    char *dt_fn = NULL;
    void *dt_data = NULL;
    uint32_t pagesize = 2048;
    int fd;
    uint32_t base           = 0x10000000U;
    uint32_t kernel_offset  = 0x00008000U;
    uint32_t ramdisk_offset = 0x01000000U;
    uint32_t second_offset  = 0x00f00000U;
    uint32_t tags_offset    = 0x00000100U;
    uint32_t kernel_sz      = 0;
    uint32_t ramdisk_sz     = 0;
    uint32_t second_sz      = 0;
    uint32_t dt_sz          = 0;
    uint32_t rec_dtbo_sz    = 0;
    uint64_t rec_dtbo_offset= 0;
    uint32_t header_sz      = 0;

    size_t cmdlen;
    enum hash_alg hash_alg = HASH_SHA1;

    argc--;
    argv++;

    memset(&hdr, 0, sizeof(hdr));

    bool get_id = false;
    while(argc > 0){
        char *arg = argv[0];
        if(!strcmp(arg, "--id")) {
            get_id = true;
            argc -= 1;
            argv += 1;
        } else if(argc >= 2) {
            char *val = argv[1];
            argc -= 2;
            argv += 2;
            if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
                bootimg = val;
            } else if(!strcmp(arg, "--kernel")) {
                kernel_fn = val;
            } else if(!strcmp(arg, "--ramdisk")) {
                ramdisk_fn = val;
            } else if(!strcmp(arg, "--second")) {
                second_fn = val;
            } else if(!strcmp(arg, "--recovery_dtbo")) {
                recovery_dtbo_fn = val;
            } else if(!strcmp(arg, "--cmdline")) {
                cmdline = val;
            } else if(!strcmp(arg, "--base")) {
                base = strtoul(val, 0, 16);
            } else if(!strcmp(arg, "--kernel_offset")) {
                kernel_offset = strtoul(val, 0, 16);
            } else if(!strcmp(arg, "--ramdisk_offset")) {
                ramdisk_offset = strtoul(val, 0, 16);
            } else if(!strcmp(arg, "--second_offset")) {
                second_offset = strtoul(val, 0, 16);
            } else if(!strcmp(arg, "--tags_offset")) {
                tags_offset = strtoul(val, 0, 16);
            } else if(!strcmp(arg, "--board")) {
                board = val;
            } else if(!strcmp(arg,"--pagesize")) {
                pagesize = strtoul(val, 0, 10);
                if ((pagesize != 2048) && (pagesize != 4096)
                    && (pagesize != 8192) && (pagesize != 16384)
                    && (pagesize != 32768) && (pagesize != 65536)
                    && (pagesize != 131072)) {
                    fprintf(stderr,"error: unsupported page size %d\n", pagesize);
                    return -1;
                }
            } else if(!strcmp(arg, "--dt")) {
                dt_fn = val;
            } else if(!strcmp(arg, "--os_version")) {
                os_version = parse_os_version(val);
            } else if(!strcmp(arg, "--os_patch_level")) {
                os_patch_level = parse_os_patch_level(val);
            } else if(!strcmp(arg, "--header_version")) {
                header_version = strtoul(val, 0, 10);
            } else if(!strcmp(arg, "--hash")) {
                hash_alg = parse_hash_alg(val);
                if (hash_alg == HASH_UNKNOWN) {
                    fprintf(stderr, "error: unknown hash algorithm '%s'\n", val);
                    return -1;
                }
            } else {
                return usage();
            }
        } else {
            return usage();
        }
    }
    hdr.page_size = pagesize;

    hdr.kernel_addr =  base + kernel_offset;
    hdr.ramdisk_addr = base + ramdisk_offset;
    hdr.second_addr =  base + second_offset;
    hdr.tags_addr =    base + tags_offset;

    hdr.os_version = (os_version << 11) | os_patch_level;
    hdr.header_version = header_version;

    if(bootimg == 0) {
        fprintf(stderr,"error: no output filename specified\n");
        return usage();
    }

    if(kernel_fn == 0) {
        fprintf(stderr,"error: no kernel image specified\n");
        return usage();
    }

    if(strlen(board) >= BOOT_NAME_SIZE) {
        fprintf(stderr,"error: board name too large\n");
        return usage();
    }

    strcpy((char *) hdr.name, board);

    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    cmdlen = strlen(cmdline);
    if(cmdlen <= BOOT_ARGS_SIZE) {
        strcpy((char *)hdr.cmdline, cmdline);
    } else if(cmdlen <= BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE) {
        /* exceeds the limits of the base command-line size, go for the extra */
        memcpy(hdr.cmdline, cmdline, BOOT_ARGS_SIZE);
        strcpy((char *)hdr.extra_cmdline, cmdline+BOOT_ARGS_SIZE);
    } else {
        fprintf(stderr,"error: kernel commandline too large\n");
        return 1;
    }

    kernel_data = load_file(kernel_fn, &kernel_sz);
    if(kernel_data == 0) {
        fprintf(stderr,"error: could not load kernel '%s'\n", kernel_fn);
        return 1;
    }
    hdr.kernel_size = kernel_sz;

    if(ramdisk_fn == NULL) {
        ramdisk_data = 0;
    } else {
        ramdisk_data = load_file(ramdisk_fn, &ramdisk_sz);
        if(ramdisk_data == 0) {
            fprintf(stderr,"error: could not load ramdisk '%s'\n", ramdisk_fn);
            return 1;
        }
    }
    hdr.ramdisk_size = ramdisk_sz;

    if(second_fn) {
        second_data = load_file(second_fn, &second_sz);
        if(second_data == 0) {
            fprintf(stderr,"error: could not load secondstage '%s'\n", second_fn);
            return 1;
        }
    }
    hdr.second_size = second_sz;

    if(header_version == 0) {
        if(dt_fn) {
            dt_data = load_file(dt_fn, &dt_sz);
            if(dt_data == 0) {
                fprintf(stderr,"error: could not load device tree image '%s'\n", dt_fn);
                return 1;
            }
        }
        hdr.dt_size = dt_sz; /* overrides hdr.header_version */
    } else {
        if(recovery_dtbo_fn) {
            recovery_dtbo_data = load_file(recovery_dtbo_fn, &rec_dtbo_sz);
            if(recovery_dtbo_data == 0) {
                fprintf(stderr,"error: could not load recovery dtbo image '%s'\n", recovery_dtbo_fn);
                return 1;
            }
            /* header occupies a page */
            rec_dtbo_offset = pagesize * (1 + \
                                          (kernel_sz + pagesize - 1) / pagesize + \
                                          (ramdisk_sz + pagesize - 1) / pagesize + \
                                          (second_sz + pagesize - 1) / pagesize);
        }
        header_sz = sizeof(hdr);
    }
    hdr.recovery_dtbo_size = rec_dtbo_sz;
    hdr.recovery_dtbo_offset = rec_dtbo_offset;
    hdr.header_size = header_sz;

    /* put a hash of the contents in the header so boot images can be
     * differentiated based on their first 2k.
     */
    generate_id(hash_alg, &hdr, kernel_data, ramdisk_data, second_data, dt_data, recovery_dtbo_data);

    fd = open(bootimg, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if(fd < 0) {
        fprintf(stderr,"error: could not create '%s'\n", bootimg);
        return 1;
    }

    if(write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) goto fail;
    if(write_padding(fd, pagesize, sizeof(hdr))) goto fail;

    if(write(fd, kernel_data, hdr.kernel_size) != (ssize_t) hdr.kernel_size) goto fail;
    if(write_padding(fd, pagesize, hdr.kernel_size)) goto fail;

    if(write(fd, ramdisk_data, hdr.ramdisk_size) != (ssize_t) hdr.ramdisk_size) goto fail;
    if(write_padding(fd, pagesize, hdr.ramdisk_size)) goto fail;

    if(second_data) {
        if(write(fd, second_data, hdr.second_size) != (ssize_t) hdr.second_size) goto fail;
        if(write_padding(fd, pagesize, hdr.second_size)) goto fail;
    }

    if(dt_data) {
        if(write(fd, dt_data, hdr.dt_size) != (ssize_t) hdr.dt_size) goto fail;
        if(write_padding(fd, pagesize, hdr.dt_size)) goto fail;
    } else if(recovery_dtbo_data) {
        if(write(fd, recovery_dtbo_data, hdr.recovery_dtbo_size) != (ssize_t) hdr.recovery_dtbo_size) goto fail;
        if(write_padding(fd, pagesize, hdr.recovery_dtbo_size)) goto fail;
    }

    if(get_id) {
        print_id((uint8_t *) hdr.id, sizeof(hdr.id));
    }
    return 0;

fail:
    unlink(bootimg);
    close(fd);
    fprintf(stderr,"error: failed writing '%s': %s\n", bootimg,
            strerror(errno));
    return 1;
}
