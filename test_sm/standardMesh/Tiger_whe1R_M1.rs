subshader "Tiger_whe1R_M1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
      alphaTestRef 0.9;
	texture "texture/TiWhe1_z";
}

subshader "Tiger_whe1R_M1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/tankhatch_h";
}

subshader "Tiger_whe1R_M1_Material2" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	twosided true;
      alphaTestRef 0.9;
	texture "texture/TiWhe1_z";
}

