WINE_INCLUDE_PATH="/usr/local/include/wine/windows"
CC=i586-mingw32msvc-cc -I$(WINE_INCLUDE_PATH)

EXECS= joystick.exe forcefeedback.exe
FLAGS=-Wall

all: $(EXECS)

joystick.exe: main.c
	$(CC) -o $@ $^ -ldinput8 -ldxguid $(FLAGS)

forcefeedback.exe: ff.c
	$(CC) -o $@ $^ -ldinput8 -ldxguid $(FLAGS)

joystickid.exe: joystickid.c
	$(CC) -o $@ $^ -ldinput8 -ldxguid $(FLAGS)

clean:
	rm *.exe
