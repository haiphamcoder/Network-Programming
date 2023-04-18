#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
    FILE *fp = fopen("text.txt", "r");
    if (fp == NULL)
    {
        perror("fopen() failed");
        return 1;
    }
    while(!feof(fp)){
        char buffer[20];
        memset(buffer, 0, 20);
        fgets(buffer, 20, fp);
        printf("%s\n", buffer);
    }
    return 0;
}