if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi
g++ -g -O0 -I . -o bin/interrupts interrupts_101311227_101268686.cpp