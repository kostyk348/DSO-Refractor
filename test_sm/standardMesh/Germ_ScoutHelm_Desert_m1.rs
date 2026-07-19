subshader "Germ_ScoutHelm_Desert_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/ghelmscout_Desert_o";
}

subshader "Germ_ScoutHelm_Desert_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	transparent true;
	twosided true;
	depthWrite false;
	texture "texture/Desertscoutleaf_s";
}

