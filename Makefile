WINE_INCLUDE_PATH="/usr/local/include/wine/windows"
CC=i586-mingw32msvc-cc -I$(WINE_INCLUDE_PATH)

joystick.exe: main.c
	$(CC) -o $@ $^ -ldinput8 -ldxguid
copy:
	cp *.exe  ~/Desktop/Dropbox/windows

clean:
	rm *.exe
