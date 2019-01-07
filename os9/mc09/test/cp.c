#include "mclibos9.c"

int
main(argc,argv) 
int argc;char *argv[]; {
    FILE *input ; 
    FILE *output ; 
    int c;
    int i;

    input = stdin;
    output = stdout;

    i = 1;
    if (argv[i]) { input = fopen(argv[i++],"r"); }
    if (argv[i]) { output = fopen(argv[i++],"w"); }
    if (input==0 || output==0) return 0;
    while( (c = getc(input) ) !=  -1) {
        putc(c,output);
    }
    return 1;
}
