subshader "PanzerIV_Whe1L_M1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/p4Whe1_f";
}

subshader "PanzerIV_Whe1L_M1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/tankhatch_h";
}

subshader "PanzerIV_Whe1L_M1_Material2" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	twosided true;
      alphaTestRef 0.7;
	texture "texture/p4Whe1_f";
}

