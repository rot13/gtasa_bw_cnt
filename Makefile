all: gtasa_bw_cnt.exe

gtasa_bw_cnt.exe : gtasa_bw_cnt.c
	i686-w64-mingw32-gcc -std=c99 gtasa_bw_cnt.c -o gtasa_bw_cnt.exe -s -mwindows
clean:
	rm *.exe
