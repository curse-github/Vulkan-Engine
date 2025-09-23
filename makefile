ifeq ($(OS),Windows_NT)
LIB_DIR = ./Lib
CONST_ARGS = -D_WINDOWS=1
else
LIB_DIR = ./Lib/Linux
CONST_ARGS = -D_LINUX=1
endif
CONST_ARGS += -D_DEBUG=1

SPV_BUILD = $(VULKAN_SDK)/Bin/glslc -o
O_BUILD = g++ -O3 -march=native -funroll-loops -I./Include -I./Include/Eng -I./Lib/Include -I$(VULKAN_SDK)/Include $(CONST_ARGS) -o
EXE_BUILD = g++ -O3 -march=native -funroll-loops -L$(LIB_DIR) -L$(VULKAN_SDK)/Lib -o

Files = app Engine Camera Loaders Helpers
EngineFiles = Eng/ECS Eng/Window Eng/Pipeline Eng/Swapchain Eng/Renderers Eng/Mesh Eng/Buffer Eng/Descriptors Eng/Device Eng/Texture Eng/RenderSystem
allFiles = $(Files) $(EngineFiles)
Shaders = Diffuse-Blinn-Phong.vert Diffuse-Blinn-Phong.frag PointLight.vert PointLight.frag FullScreen.vert OnTilePostProcess.frag OffTilePostProcess.frag

./out/%.o: makefolders ./Include/%.h | ./Src/%.cpp
	$(O_BUILD) $@ -c $|
./out/shaders/%.spv: makefolders | ./Src/shaders/%
	$(SPV_BUILD) $@ $|

makefolders:
	@-mkdir out
ifeq ($(OS),Windows_NT)
	@-mkdir out\Eng
	@-mkdir out\shaders
else
	@-mkdir out/Eng
	@-mkdir out/shaders
endif
shaders: $(Shaders:%=./out/shaders/%.spv)
./out/app.exe: makefolders shaders | $(allFiles:%=./out/%.o)
	$(EXE_BUILD) $@ $| -lglfw3 -lvulkan -lgdi32
./out/app.out: makefolders shaders | $(allFiles:%=./out/%.o)
	$(EXE_BUILD) $@ $| -lglfw3 -lvulkan
clean:
ifeq ($(OS),Windows_NT)
	rmdir /s /q out
else
	rm -Rf ./out
endif
