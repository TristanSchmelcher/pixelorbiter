find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (pixelorbiter PLUGINDEPS composite opengl mousepoll)
