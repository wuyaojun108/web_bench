src=main.c web_bench.h web_bench.c
target=wb

${target}: ${src}
	$(CC) -g ${src} -lpcre -o ${target} -Wall -Wextra -Wpedantic -std=c99
