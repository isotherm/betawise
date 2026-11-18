CRT_BUILTINS = compiler-rt/upstream
CRT_NAMES = fp_mode muldi3 ashldi3 ashrdi3 lshrdi3 negdi2 cmpdi2 ucmpdi2 \
    divsi3 udivsi3 modsi3 umodsi3 divmodsi4 udivmodsi4 \
    divdi3 udivdi3 moddi3 umoddi3 divmoddi4 udivmoddi4 \
    clzsi2 clzdi2 ctzsi2 ctzdi2 ffssi2 ffsdi2 popcountsi2 popcountdi2 \
    paritysi2 paritydi2 bswapsi2 bswapdi2 \
    addsf3 subsf3 mulsf3 divsf3 negsf2 comparesf2 \
    adddf3 subdf3 muldf3 divdf3 negdf2 comparedf2 extendsfdf2 truncdfsf2 \
    floatsisf floatunsisf floatdisf floatundisf \
    floatsidf floatunsidf floatdidf floatundidf \
    fixsfsi fixunssfsi fixsfdi fixunssfdi fixdfsi fixunsdfsi fixdfdi fixunsdfdi
CRT_OBJECTS = $(addprefix compiler-rt/build/,$(addsuffix .o,$(CRT_NAMES) mulsi3))
CRT_HEADERS = $(wildcard $(CRT_BUILTINS)/*.h $(CRT_BUILTINS)/*.inc)

compiler-rt/build/%.o: $(CRT_BUILTINS)/%.c $(CRT_HEADERS) compiler-rt/builtins.mk ../Makefile.common Makefile
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -ffreestanding -c $< -o $@

compiler-rt/build/mulsi3.o: compiler-rt/mulsi3.c compiler-rt/builtins.mk ../Makefile.common Makefile
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -ffreestanding -c $< -o $@
