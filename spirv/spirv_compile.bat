glslc.exe -c -fshader-stage=compute ../shader2.glsl -o ../sky.spv
glslc.exe -c ../gradient_color.comp -o ../gradient_color.spv
echo "done"
pause