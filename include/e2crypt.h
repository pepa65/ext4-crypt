// e2crypt.h

extern char *contents_cipher;
extern char *filename_cipher;
extern unsigned padding;

#ifndef EXT4_CRYPT_H
#define EXT4_CRYPT_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <asm-generic/ioctl.h>
#include <keyutils.h>

#define NAME "e2crypt"
#define EXT4_KEY_DESCRIPTOR_SIZE 8
#define EXT4_ENCRYPTION_CONTEXT_FORMAT_V1 1
#define EXT4_KEY_DERIVATION_NONCE_SIZE 16

#define EXT4_POLICY_FLAGS_PAD_4 0x00
#define EXT4_POLICY_FLAGS_PAD_8 0x01
#define EXT4_POLICY_FLAGS_PAD_16 0x02
#define EXT4_POLICY_FLAGS_PAD_32 0x03
#define EXT4_POLICY_FLAGS_PAD_MASK 0x03
#define EXT4_POLICY_FLAGS_VALID 0x03

#define EXT4_ENCRYPTION_MODE_INVALID 0
#define EXT4_ENCRYPTION_MODE_AES_256_XTS 1
#define EXT4_ENCRYPTION_MODE_AES_256_GCM 2
#define EXT4_ENCRYPTION_MODE_AES_256_CBC 3
#define EXT4_ENCRYPTION_MODE_AES_256_CTS 4

#define EXT4_XTS_TWEAK_SIZE 16
#define EXT4_AES_128_ECB_KEY_SIZE 16
#define EXT4_AES_256_GCM_KEY_SIZE 32
#define EXT4_AES_256_CBC_KEY_SIZE 32
#define EXT4_AES_256_CTS_KEY_SIZE 32
#define EXT4_AES_256_XTS_KEY_SIZE 64
#define EXT4_MAX_KEY_SIZE 64
#define EXT4_KEY_DESC_PREFIX "ext4:"
#define EXT4_KEY_DESC_PREFIX_SIZE 5

#define EXT4_IOC_SET_ENCRYPTION_POLICY _IOR('f', 19, struct ext4_encryption_policy)
#define EXT4_IOC_GET_ENCRYPTION_PWSALT _IOW('f', 20, __u8[16])
#define EXT4_IOC_GET_ENCRYPTION_POLICY _IOW('f', 21, struct ext4_encryption_policy)

#define EXT4_MAX_PASSPHRASE_SZ 128
#define EXT4_ENCRYPTION_KEY_TYPE "logon"
#define EXT4_FULL_KEY_DESCRIPTOR_SIZE (EXT4_KEY_DESCRIPTOR_SIZE * 2 + EXT4_KEY_DESC_PREFIX_SIZE)

typedef char key_desc_t[EXT4_KEY_DESCRIPTOR_SIZE];
typedef char full_key_desc_t[EXT4_FULL_KEY_DESCRIPTOR_SIZE];

// Policy provided via an ioctl on the topmost directory
struct ext4_encryption_policy {
    char version;
    char contents_encryption_mode;
    char filenames_encryption_mode;
    char flags;
//    char master_key_descriptor[EXT4_KEY_DESCRIPTOR_SIZE];
    key_desc_t master_key_descriptor;
} __attribute__((__packed__));

struct ext4_encryption_key {
    uint32_t mode;
    unsigned char raw[EXT4_MAX_KEY_SIZE];
    uint32_t size;
};

static inline
char padding_length_to_flags(unsigned padding)
{
    switch (padding) {
        case 4: return EXT4_POLICY_FLAGS_PAD_4;
        case 8: return EXT4_POLICY_FLAGS_PAD_8;
        case 16: return EXT4_POLICY_FLAGS_PAD_16;
        case 32: return EXT4_POLICY_FLAGS_PAD_32;
        default: fprintf(stderr, "Invalid padding value: %d\n", padding);
            abort();
    }
}

static inline
unsigned flags_to_padding_length(char flags)
{
    return (4 << (flags & EXT4_POLICY_FLAGS_PAD_MASK));
}

struct cipher {
    const char *cipher_name;
    size_t cipher_key_size;
};

static
struct cipher cipher_modes[] = {
    [EXT4_ENCRYPTION_MODE_INVALID] = { "invalid", 0 },
    [EXT4_ENCRYPTION_MODE_AES_256_XTS] = { "aes-256-xts", EXT4_AES_256_XTS_KEY_SIZE },
    [EXT4_ENCRYPTION_MODE_AES_256_GCM] = { "aes-256-gcm", EXT4_AES_256_GCM_KEY_SIZE },
    [EXT4_ENCRYPTION_MODE_AES_256_CBC] = { "aes-256-cbc", EXT4_AES_256_CBC_KEY_SIZE },
    [EXT4_ENCRYPTION_MODE_AES_256_CTS] = { "aes-256-cts", EXT4_AES_256_CTS_KEY_SIZE },
};

#define NR_EXT4_ENCRYPTION_MODES (sizeof(cipher_modes) / sizeof(cipher_modes[0]))

static inline
const char *cipher_mode_to_string(unsigned char mode)
{
    if (mode >= NR_EXT4_ENCRYPTION_MODES) return "invalid";
    return cipher_modes[mode].cipher_name;
}

static inline
char cipher_string_to_mode(const char *cipher)
{
    for (size_t i = 0; i < NR_EXT4_ENCRYPTION_MODES; i++)
        if (strcmp(cipher, cipher_modes[i].cipher_name) == 0) return i;
    fprintf(stderr, "Invalid cipher mode: %s\n", cipher);
    abort();
}

static inline
size_t cipher_key_size(const char *cipher)
{
    for (size_t i = 0; i < NR_EXT4_ENCRYPTION_MODES; i++)
        if (strcmp(cipher, cipher_modes[i].cipher_name) == 0)
            return cipher_modes[i].cipher_key_size;
    fprintf(stderr, "Invalid cipher mode: %s\n", cipher);
    abort();
}

int crypto_init();
int container_status(const char *);
int container_create(const char *);
int container_attach(const char *);
int container_detach(const char *);
void generate_random_name(char *, size_t, bool);
int find_key_by_descriptor(key_desc_t *, key_serial_t *);
int request_key_for_descriptor(key_desc_t *, bool);
int remove_key_for_descriptor(key_desc_t *);
void error(bool, const char *, ...);

#endif
