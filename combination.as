	lw	0       1       n	r1 = n
        lw	0       2       r	r2 = r
        lw	0       4       Caddr	load combination function address
        jalr	4	7		call function combo
        halt
combo   lw	0	6	pos1	r6 = 1
	sw	5	7	Stack	put return adress on stack
	add	6	5	5	stack++
	sw	5	1	Stack	put n on stack
	add	6	5	5	stack++
	sw	5	2	Stack	put r on stack
	add 	6	5	5	stack++
	beq	2	0	if1	if r = 0, plop 1 into return reg
	beq	1	2	if1	if r = n, plop 1 into return reg
	lw	0	6	neg1	r6 = -1
	add	1	6	1	decrement n	
	jalr	4	7		recurse func w/ n-1, r
	lw	0	6	pos1	r6 = 1
	sw	5	3	Stack	store the return value on stack
	add	6	5	5	increment stack ptr
	lw	0	6	neg1	r6 = -1
	add	2	6	2	decrement r
	jalr	4	7		recurse func w/ n-1, r-1
	lw	0	6	neg1	r6 = -1
	add	6	5	5	decrement stack ptr
	lw	5	6	Stack	retreive other recrusive return value
	add	3	6	3	add return values together
	beq	0	0	1	go to 30 to skip the return 1	
if1	lw	0	3	pos1	return 1
	lw	0	6	neg1	r6 = -1
	add	6	5	5	stack--
	lw	5	2	Stack	grab r from stack
	add	6	5	5	stack--
	lw	5	1	Stack	grab n from stack
	add	6	5	5	stack--
	lw	5	7	Stack	grab return value from stack
	jalr	7	6		return, no need to store this addy
n       .fill	7
r       .fill	3
Caddr   .fill	combo
pos1	.fill	1
neg1	.fill	-1
Stack   .fill	0