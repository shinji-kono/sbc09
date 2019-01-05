#!/usr/bin/perl

my $indent = 7;
while(<>) {
    s/\r\n$//;
    s/^\d+ //;
    s/^ /" "x$indent/e;
    if (/^([a-zA-Z0-9]+) /) {
        my $w = $1;
        if (length $w < $indent) {
            my $s = " "x($indent-length $w);
            s/ /$s/e;
        }
    }
    print $_,"\n" 
}
