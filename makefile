.PHONY: openal
openal:
	@clang++ -framework OpenAL -o openal.exe openal.cpp  -g -std=c++11 -O3 && ./openal.exe

openal-debug:
	@clang++ -framework OpenAL -o openal-d.exe openal.cpp -g -std=c++11