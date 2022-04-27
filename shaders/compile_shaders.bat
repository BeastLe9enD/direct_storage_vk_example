@echo off
glslangValidator -V example.vert.glsl -o ../bin/example.vert.spv
glslangValidator -V example.frag.glsl -o ../bin/example.frag.spv