subshader "US_ScoutHelm_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	texture "texture/ahelmscout_o";
}

subshader "US_ScoutHelm_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 0.5;
	transparent true;
	twosided true;
	depthWrite false;
	texture "texture/scoutcover01_o";
}

