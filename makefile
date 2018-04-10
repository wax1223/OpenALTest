.PHONY: openal
openal:
	@clang++ -framework OpenAL -o openal.exe openal.cpp && ./openal.exe