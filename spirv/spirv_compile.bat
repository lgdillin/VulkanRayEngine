glslc.exe -c ../triangle.vert -o ../triangle.vert.spv
glslc.exe -c ../triangle.frag -o ../triangle.frag.spv
echo "done"
spirv-val.exe ../triangle.frag.spv
pause