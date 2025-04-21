game_engine_linux:
	clang++ -O3 src/*.cpp Third_Party/box2d/collision/*.cpp Third_Party/box2d/common/*.cpp Third_Party/box2d/dynamics/*.cpp Third_Party/box2d/rope/*.cpp -std=c++17 -I./src -I./Third_Party -I./Third_Party/glm-0.9.9.8 -I./Third_Party/rapidjson-1.1.0/rapidjson-1.1.0/include -I./Third_Party/SDL/ -I./Third_Party/Lua/ -I./Third_Party/box2d/ -I./Third_Party/box2d/dynamics -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -llua5.4 -o game_engine_linux

clean:
	rm -f game_engine_linux