subshader "mp18_main_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.117647 0.117647 0.117647;
	materialSpecularPower 12.5;
	envmap true;
	texture "texture/jap_mgun_T ";
}

subshader "mp18_main_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	transparent true;
	depthwrite false;
	texture "texture/jap_mgun_T ";
}

