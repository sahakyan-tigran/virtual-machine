; Sums 1..10 and prints the result (55).
;
; The machine has no branch instruction: a program jumps by writing to the
; counter. To pick between two targets it computes a mask from the condition
; (0 - 1 is all ones, 0 - 0 is zero) and ANDs each target with it.
;
;   python tools/assemble.py examples/sum_to_ten.asm sum_to_ten.bin
;   ./virtualmachine sum_to_ten.bin

        alu  add  #0, #0   -> r0      ; sum = 0
        alu  add  #1, #0   -> r1      ; i = 1

loop:
        alu  add  r0, r1   -> r0      ; sum += i
        alu  add  r1, #1   -> r1      ; i += 1

        cond le   r1, #10  -> r2      ; r2 = (i <= 10)
        alu  sub  #0, r2   -> r3      ; r3 = -r2       -1 if looping, else 0
        alu  sub  r2, #1   -> r4      ; r4 = r2 - 1     0 if looping, else -1
        alu  and  r3, @loop -> r3     ; r3 = loop target, or 0
        alu  and  r4, @done -> r4     ; r4 = done target, or 0
        alu  or   r3, r4   -> counter ; jump to whichever survived

done:
        alu  add  r0, #0   -> out     ; print the sum
