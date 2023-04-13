#include <stdio.h>
#include <string.h>

int main()
{
    char str[256];
    printf("Nhap xau ky tu: ");
    fgets(str, sizeof(str), stdin);

    char cmd[16], tmp[16];
    float x, y;
    int n = sscanf(str, "%s%f%f%s", cmd, &x, &y, tmp);
    printf("n = %d\n", n);

    if (n < 3)
    {
        printf("ERROR thieu tham so\n");
        return 1;
    }

    if (n > 3)
    {
        printf("ERROR thua tham so\n");
        return 1;
    }

    if (strcmp(cmd, "ADD") != 0 && strcmp(cmd, "SUB") != 0 &&
        strcmp(cmd, "MUL") != 0 && strcmp(cmd, "DIV") != 0)
    {
        printf("ERROR sai lenh\n");
        return 1;
    }

    printf("OK dung cu phap\n");
}