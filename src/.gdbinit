handle 2 pass
define regs 
call (void)printf("rax=%08lx rbx=%08lx rcx=%08lx rdx=%08lx\nrsi=%08lx rdi=%08lx rbp=%08lx rsp=%08lx rip=%08lx\n",$rax,$rbx,$rcx,$rdx,$rsi,$rdi,$rbp,$rsp,$rip)
end
define si
stepi
regs
x/1i $rip
end
define ni
nexti
regs
x/1i $rip
end

