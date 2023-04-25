#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(){
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[100];
    memset(time_str, 0, 100);
    strftime(time_str, 100, "%Y/%m/%d %I:%M:%S%p", t);
    printf("%s\n", time_str);
    return 0;
}