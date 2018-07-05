# pixpix

确认过眼神，是我想做的项目。一个C++实现的简单光栅化渲染器。

功能未定？先把图形输出搞定再说。

`g++ ./src/main.cpp ./src/Math.cpp ./src/RenderPipeline3D.cpp -o ./bin/main.exe -O3`

2018/07/05
- Added `MESH` to represent polygons and primitives.
- `RenderPipeline3D` is refactored to support the new `MESH` structure.
- Meshes to be rendered **MUST** be triangles. Polygon face is not supported yet.

# TODO

1. ~~共用公共顶点，使用顶点index标记多边形；~~
1. 增加更多几何体，立方体、球、圆柱、犹他茶壶；
1. 规范化输入到渲染管线的数据结构，将材质、贴图、几何体的信息集成输入渲染管线；
1. 增加聚光灯、平行光等新的光线类型；
1. 增加图片纹理贴图；
1. ~~修改三角形填充算法实现，提高性能；~~
1. 增加多边形裁剪。