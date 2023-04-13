#include <stdio.h>

int main()
{
    char packet_bytes[] = {
        0x45, 0x00, 0x02, 0x04,
        0x0a, 0x8c, 0x40, 0x00,
        0x80, 0x06, 0x00, 0x00,
        0x0a, 0x5a, 0x4c, 0x96,
        0x34, 0x56, 0x44, 0x2e};

    int version, ihl, total_length;

    version = packet_bytes[0] >> 4;
    printf("version = %d\n", version);

    ihl = (packet_bytes[0] & 0xf) * 4;
    printf("ihl = %d bytes\n", ihl);

    total_length = packet_bytes[2] * 256 + packet_bytes[3];
    printf("total_length = %d bytes\n", total_length);

    printf("IP src: %u.%u.%u.%u\n",
           (unsigned char)packet_bytes[12],
           (unsigned char)packet_bytes[13],
           (unsigned char)packet_bytes[14],
           (unsigned char)packet_bytes[15]);

    printf("IP dst: %u.%u.%u.%u\n",
           (unsigned char)packet_bytes[16],
           (unsigned char)packet_bytes[17],
           (unsigned char)packet_bytes[18],
           (unsigned char)packet_bytes[19]);
}