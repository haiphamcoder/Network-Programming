#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    char str[] = "227 Entering Passive Mode (213,229,112,130,216,4)";

    unsigned char i1, i2, i3, i4;
    unsigned char p1, p2;

    char *pos = strchr(str, '(');
    char *p = strtok(pos, "(),");
    i1 = atoi(p);
    p = strtok(NULL, "(),");
    i2 = atoi(p); 
    p = strtok(NULL, "(),");
    i3 = atoi(p);
    p = strtok(NULL, "(),");
    i4 = atoi(p);

    p = strtok(NULL, "(),");
    p1 = atoi(p);
    p = strtok(NULL, "(),");
    p2 = atoi(p);

    printf("%u.%u.%u.%u:%d\n", i1, i2, i3, i4, p1 * 256 + p2);
}