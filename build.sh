#!/bin/sh
# find all C sources under the current directory, compile and link them.
# So many projects can be built with such a simple build script...

OBJS=""
CFLAGS="-O0 -Wall -Wextra -Winline -Wstrict-aliasing -Wstrict-overflow -Wmissing-include-dirs -pedantic-errors -I ./headers"
CC="gcc"

for F in $(find ./ -name "*.c")
do
	$CC $CFLAGS -c "${F}" -o "${F%.c}.o"
	OBJS="${OBJS} ${F%.c}.o"
done

# Yes, we're using GCC to link. I still find it kind of ridiculous
# that a compiler is easier to use for linking than... you know... a *linker*.
# Though I guess it kinda makes sense that a compiler would have more ready access to
# system-wide configuration and library locations.
$CC $OBJS -o BlockChain.elf
