.PHONY: openal
openal:
	@clang++ -framework OpenAL -o openal.exe openal.cpp -std=c++11 && ./openal.exe

openal-debug:
	@clang++ -framework OpenAL -o openal-d.exe openal.cpp -g -std=c++11